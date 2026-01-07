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

NetworkWorker::NetworkWorker(QObject *parent) : QObject(parent)
{
    socket = new QTcpSocket(this);
    retryTimer = new QTimer(this);

    connect(socket, &QTcpSocket::connected, this, &NetworkWorker::onConnected);
    connect(socket, &QTcpSocket::readyRead, this, &NetworkWorker::onReadyRead);
    connect(socket, &QTcpSocket::errorOccurred, this, &NetworkWorker::onError);
    connect(retryTimer, &QTimer::timeout, this, &NetworkWorker::connectToServer);
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
    // 发送简单指令，触发服务器返回数据
    socket->write("GET_SCHEDULE");
}

void NetworkWorker::onReadyRead() {
    buffer.append(socket->readAll());
    qDebug() << "接收到数据，当前缓冲区大小:" << buffer.size();

    if (buffer.endsWith('\n')) {
        qDebug() << "数据接收完成，开始解析...";
        qDebug() << "接收到的JSON数据:" << buffer;
        updateLocalDb(buffer);
        buffer.clear();
        socket->disconnectFromHost();
    }
}

void NetworkWorker::onError(QAbstractSocket::SocketError socketError) {
    Q_UNUSED(socketError);
    qDebug() << "网络错误/服务器离线，保持本地数据显示。Error:" << socket->errorString();
    socket->abort();
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
    QSqlDatabase db = QSqlDatabase::database();
    if (!db.isOpen()) {
        qDebug() << "数据库未打开";
        return;
    }

    db.transaction();

    QSqlQuery query(db);
    query.exec("DELETE FROM schedules");
    qDebug() << "已清空课程表数据";

    QString sql = "INSERT INTO schedules (room_name, course_name, teacher, time_slot, is_next) VALUES (?, ?, ?, ?, ?)";
    query.prepare(sql);

    int successCount = 0;
    for (const QJsonValue &value : array) {
        QJsonObject obj = value.toObject();

        query.addBindValue(obj["room_name"].toString());
        query.addBindValue(obj["course_name"].toString());
        query.addBindValue(obj["teacher"].toString());
        query.addBindValue(obj["time_slot"].toString());
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
    QSqlDatabase db = QSqlDatabase::database();
    if (!db.isOpen()) {
        qDebug() << "数据库未打开";
        return;
    }

    db.transaction();

    QSqlQuery query(db);
    query.exec("DELETE FROM classrooms");
    qDebug() << "已清空教室数据";

    QString sql = "INSERT INTO classrooms (room_name, class_name, capacity, building, floor) VALUES (?, ?, ?, ?, ?)";
    query.prepare(sql);

    int successCount = 0;
    for (const QJsonValue &value : array) {
        QJsonObject obj = value.toObject();

        query.addBindValue(obj["room_name"].toString());
        query.addBindValue(obj["class_name"].toString());
        query.addBindValue(obj["capacity"].toInt());
        query.addBindValue(obj["building"].toString());
        query.addBindValue(obj["floor"].toInt());

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
    QSqlDatabase db = QSqlDatabase::database();
    if (!db.isOpen()) {
        qDebug() << "数据库未打开";
        return;
    }

    db.transaction();

    QSqlQuery query(db);
    query.exec("DELETE FROM announcements");
    qDebug() << "已清空公告数据";

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
