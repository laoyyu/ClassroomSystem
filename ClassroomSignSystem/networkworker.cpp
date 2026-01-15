#include "networkworker.h"
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QDateTime>
#include <QDebug>
#include <QThread>
#include <QDataStream>

NetworkWorker::NetworkWorker(QObject *parent) : QObject(parent), expectedDataSize(0), receivingData(false)
{
    socket = new QTcpSocket(this);
    retryTimer = new QTimer(this);
    receiveTimer = new QTimer(this);

    socket->setReadBufferSize(50 * 1024 * 1024);

    connect(socket, &QTcpSocket::connected, this, &NetworkWorker::onConnected);
    connect(socket, &QTcpSocket::readyRead, this, &NetworkWorker::onReadyRead);
    connect(socket, &QTcpSocket::errorOccurred, this, &NetworkWorker::onError);
    connect(retryTimer, &QTimer::timeout, this, &NetworkWorker::connectToServer);
    connect(receiveTimer, &QTimer::timeout, this, &NetworkWorker::onReceiveTimeout);

    receiveTimer->setSingleShot(true);
}

void NetworkWorker::startSync() {
    retryTimer->start(10000);

    connectToServer();
}

void NetworkWorker::connectToServer() {
    // 如果正在连接或已连接，跳过
    if(socket->state() == QAbstractSocket::ConnectedState ||
        socket->state() == QAbstractSocket::ConnectingState) {
        return;
    }

    qDebug() << "正在连接服务器...";
    // 连接本地服务器 (127.0.0.1:12345)
    socket->connectToHost("127.0.0.1", 12345);
}

void NetworkWorker::onConnected() {
    qDebug() << "已连接服务器，发送同步请求...";

    // 重置接收状态
    buffer.clear();
    expectedDataSize = 0;
    receivingData = true;

    // 启动接收超时定时器（30秒）
    receiveTimer->start(30000);

    socket->write("GET_SCHEDULE");
    socket->flush();
    qDebug() << "请求已发送";
}

void NetworkWorker::onReadyRead() {
    if (!receivingData) {
        qDebug() << "警告：在非接收状态下收到数据，忽略";
        return;
    }

    QByteArray data = socket->readAll();
    buffer.append(data);
    qDebug() << "接收到数据，本次接收:" << data.size() << "字节，当前缓冲区大小:" << buffer.size();

    // 重置接收超时计时器（有新数据到达）
    receiveTimer->start(30000);

    // 解析长度头（4字节大端 quint32）
    if (expectedDataSize == 0 && buffer.size() >= 4) {
        // 使用临时缓冲区解析长度，避免直接操作 buffer
        QByteArray sizeHeader = buffer.left(4);
        QDataStream ds(sizeHeader);
        ds.setByteOrder(QDataStream::BigEndian);
        quint32 size;
        ds >> size;
        expectedDataSize = static_cast<qint32>(size);
        qDebug() << "预期数据大小:" << expectedDataSize << "字节";

        // 移除长度头
        buffer.remove(0, 4);
        qDebug() << "移除头部后缓冲区大小:" << buffer.size() << "字节";
    }

    // 检查是否接收完整
    if (expectedDataSize > 0 && buffer.size() >= expectedDataSize) {
        // 停止超时计时器
        receiveTimer->stop();

        QByteArray jsonData = buffer.left(expectedDataSize);
        qDebug() << "数据接收完成，实际接收:" << jsonData.size() << "字节，预期:" << expectedDataSize << "字节";
        qDebug() << "Socket状态:" << socket->state();

        // 处理数据
        updateLocalDb(jsonData);

        // 清理状态
        buffer.remove(0, expectedDataSize);
        expectedDataSize = 0;
        receivingData = false;

        qDebug() << "准备断开连接...";
        socket->disconnectFromHost();
    } else if (expectedDataSize > 0) {
        qDebug() << "等待更多数据，已接收:" << buffer.size() << "字节，需要:" << expectedDataSize << "字节"
                 << "，剩余:" << (expectedDataSize - buffer.size()) << "字节";
    }
}

void NetworkWorker::onError(QAbstractSocket::SocketError socketError) {
    Q_UNUSED(socketError);
    qDebug() << "网络错误/服务器离线，保持本地数据显示。Error:" << socket->errorString();
    qDebug() << "Socket状态:" << socket->state();

    // 停止接收超时计时器
    receiveTimer->stop();
    receivingData = false;

    if (socket->state() != QAbstractSocket::UnconnectedState) {
        socket->disconnectFromHost();
    }
}

void NetworkWorker::onReceiveTimeout() {
    qDebug() << "警告：数据接收超时！已接收:" << buffer.size() << "字节，预期:" << expectedDataSize << "字节";

    // 清理状态
    buffer.clear();
    expectedDataSize = 0;
    receivingData = false;

    // 断开连接
    if (socket->state() != QAbstractSocket::UnconnectedState) {
        socket->disconnectFromHost();
    }
}

QSqlDatabase NetworkWorker::getDatabase()
{
    const QString connectionName = "NetworkWorkerConnection";

    QSqlDatabase db;
    if (QSqlDatabase::contains(connectionName)) {
        db = QSqlDatabase::database(connectionName);
    } else {
        db = QSqlDatabase::addDatabase("QSQLITE", connectionName);
        db.setDatabaseName("classroom.db");
    }

    if (!db.isOpen() && !db.open()) {
        qDebug() << "NetworkWorker 线程中数据库打开失败:" << db.lastError().text();
    }

    return db;
}

void NetworkWorker::updateLocalDb(const QByteArray &jsonData) {
    qDebug() << "开始解析JSON数据...";
    QJsonDocument doc = QJsonDocument::fromJson(jsonData);

    if (doc.isNull()) {
        qDebug() << "JSON解析失败，数据格式错误";
        return;
    }

    if (doc.isObject()) {
        qDebug() << "接收到对象格式的JSON数据";
        QJsonObject rootObj = doc.object();

        if (rootObj.contains("announcements")) {
            QJsonArray announcements = rootObj["announcements"].toArray();
            qDebug() << "公告数据数量:" << announcements.size();
            saveAnnouncements(announcements);
        }

        if (rootObj.contains("schedules")) {
            QJsonArray schedules = rootObj["schedules"].toArray();
            qDebug() << "课程表数据数量:" << schedules.size();
            saveSchedules(schedules);
        }

        if (rootObj.contains("classrooms")) {
            QJsonArray classrooms = rootObj["classrooms"].toArray();
            qDebug() << "教室数据数量:" << classrooms.size();
            saveClassrooms(classrooms);
        }
    } else if (doc.isArray()) {
        qDebug() << "接收到数组格式的JSON数据";
        QJsonArray array = doc.array();
        qDebug() << "数组数据数量:" << array.size();
        saveSchedules(array);
    } else {
        qDebug() << "数据格式错误，既不是对象也不是数组";
        return;
    }
}

void NetworkWorker::saveSchedules(const QJsonArray &array) {
    qDebug() << "开始保存课程表数据...";
    QSqlDatabase db = getDatabase();
    if (!db.isValid() || !db.isOpen()) {
        qDebug() << "NetworkWorker 线程中数据库不可用";
        return;
    }

    db.transaction();

    QSqlQuery query(db);
    
    // 删除旧表
    query.exec("DROP TABLE IF EXISTS schedules");
    qDebug() << "已删除旧的课程表";
    
    // 重新创建表结构
    if (!query.exec("CREATE TABLE schedules ("
                    "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                    "room_name TEXT, "
                    "course_name TEXT, "
                    "teacher TEXT, "
                    "time_slot TEXT, "
                    "start_time TEXT, "
                    "end_time TEXT, "
                    "weekday INTEGER, "
                    "is_next INTEGER DEFAULT 0)")) {
        qDebug() << "创建课程表失败:" << query.lastError().text();
        db.rollback();
        return;
    }
    qDebug() << "已重新创建课程表";

    QString sql = "INSERT INTO schedules (room_name, course_name, teacher, time_slot, start_time, end_time, weekday, is_next) VALUES (?, ?, ?, ?, ?, ?, ?, ?)";
    query.prepare(sql);

    int successCount = 0;
    for (const QJsonValue &value : array) {
        QJsonObject obj = value.toObject();

        query.addBindValue(obj["room_name"].toString());
        query.addBindValue(obj["course_name"].toString());
        query.addBindValue(obj["teacher"].toString());
        query.addBindValue(obj["time_slot"].toString());
        query.addBindValue(obj["start_time"].toString());
        query.addBindValue(obj["end_time"].toString());
        query.addBindValue(obj["weekday"].toInt());
        query.addBindValue(obj["is_next"].toInt());

        if (!query.exec()) {
            qDebug() << "插入失败:" << query.lastError().text();
        } else {
            successCount++;
        }
    }

    if (db.commit()) {
        QString timeStr = QDateTime::currentDateTime().toString("HH:mm:ss");
        emit dataUpdated("同步成功 (Server): " + timeStr);
        qDebug() << "本地数据库已更新: " << successCount << " 条记录";
    } else {
        db.rollback();
        qDebug() << "数据库写入失败";
    }
}

void NetworkWorker::saveClassrooms(const QJsonArray &array) {
    qDebug() << "开始保存教室数据...";
    QSqlDatabase db = getDatabase();
    if (!db.isValid() || !db.isOpen()) {
        qDebug() << "NetworkWorker 线程中数据库不可用";
        return;
    }

    db.transaction();

    QSqlQuery query(db);
    
    // 删除旧表
    query.exec("DROP TABLE IF EXISTS classrooms");
    qDebug() << "已删除旧的教室表";
    
    // 重新创建表结构
    if (!query.exec("CREATE TABLE classrooms ("
                    "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                    "room_name TEXT UNIQUE, "
                    "class_name TEXT, "
                    "capacity INTEGER, "
                    "building TEXT, "
                    "floor INTEGER, "
                    "current_class TEXT)")) {
        qDebug() << "创建教室表失败:" << query.lastError().text();
        db.rollback();
        return;
    }
    qDebug() << "已重新创建教室表";

    QString sql = "INSERT INTO classrooms (room_name, class_name, capacity, building, floor, current_class) VALUES (?, ?, ?, ?, ?, ?)";
    query.prepare(sql);

    int successCount = 0;
    for (const QJsonValue &value : array) {
        QJsonObject obj = value.toObject();

        query.addBindValue(obj["room_name"].toString());
        query.addBindValue(obj["class_name"].toString());
        query.addBindValue(obj["capacity"].toInt());
        query.addBindValue(obj["building"].toString());
        query.addBindValue(obj["floor"].toInt());
        query.addBindValue(obj["current_class"].toString());

        if (!query.exec()) {
            qDebug() << "插入教室数据失败:" << query.lastError().text();
        } else {
            successCount++;
        }
    }

    if (db.commit()) {
        qDebug() << "教室数据已同步: " << successCount << " 条记录";
    } else {
        db.rollback();
        qDebug() << "教室数据写入失败";
    }
}

void NetworkWorker::saveAnnouncements(const QJsonArray &array) {
    qDebug() << "开始保存公告数据...";
    QSqlDatabase db = getDatabase();
    if (!db.isValid() || !db.isOpen()) {
        qDebug() << "NetworkWorker 线程中数据库不可用";
        return;
    }

    db.transaction();

    QSqlQuery query(db);
    
    // 删除旧表
    query.exec("DROP TABLE IF EXISTS announcements");
    qDebug() << "已删除旧的公告表";
    
    // 重新创建表结构
    if (!query.exec("CREATE TABLE announcements ("
                    "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                    "title TEXT, "
                    "content TEXT, "
                    "priority INTEGER DEFAULT 0, "
                    "publish_time TEXT, "
                    "expire_time TEXT)")) {
        qDebug() << "创建公告表失败:" << query.lastError().text();
        db.rollback();
        return;
    }
    qDebug() << "已重新创建公告表";

    QString sql = "INSERT INTO announcements (title, content, priority, publish_time, expire_time) VALUES (?, ?, ?, ?, ?)";
    query.prepare(sql);

    int successCount = 0;
    for (const QJsonValue &value : array) {
        QJsonObject obj = value.toObject();

        query.addBindValue(obj["title"].toString());
        query.addBindValue(obj["content"].toString());
        query.addBindValue(obj["priority"].toInt());
        query.addBindValue(obj["publish_time"].toString());
        query.addBindValue(obj["expire_time"].toString());

        if (!query.exec()) {
            qDebug() << "插入公告数据失败:" << query.lastError().text();
        } else {
            successCount++;
        }
    }

    if (db.commit()) {
        qDebug() << "公告数据已同步: " << successCount << " 条记录";
        if (!array.isEmpty()) {
            QJsonObject ann = array.first().toObject();
            emit announcementUpdated(ann["title"].toString(), ann["content"].toString());
        }
    } else {
        db.rollback();
        qDebug() << "公告数据写入失败";
    }
}
