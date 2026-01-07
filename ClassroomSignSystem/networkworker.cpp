#include "NetworkWorker.h"
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

    if (buffer.endsWith('\n')) {
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
    QJsonDocument doc = QJsonDocument::fromJson(jsonData);
    
    if (doc.isObject()) {
        QJsonObject rootObj = doc.object();
        
        if (rootObj.contains("announcements")) {
            QJsonArray announcements = rootObj["announcements"].toArray();
            if (!announcements.isEmpty()) {
                QJsonObject ann = announcements.first().toObject();
                QString title = ann["title"].toString();
                QString content = ann["content"].toString();
                emit announcementUpdated(title, content);
            }
        }
        
        if (rootObj.contains("schedules")) {
            QJsonArray array = rootObj["schedules"].toArray();
            saveSchedules(array);
        }
    } else if (doc.isArray()) {
        QJsonArray array = doc.array();
        saveSchedules(array);
    } else {
        qDebug() << "数据格式错误";
        return;
    }
}

void NetworkWorker::saveSchedules(const QJsonArray &array) {
    QSqlDatabase db = QSqlDatabase::database();
    if (!db.isOpen()) return;

    db.transaction();

    QSqlQuery query(db);
    query.exec("DELETE FROM schedules");

    QString sql = "INSERT INTO schedules (room_name, course_name, teacher, time_slot, is_next) VALUES (?, ?, ?, ?, ?)";
    query.prepare(sql);

    for (const QJsonValue &value : array) {
        QJsonObject obj = value.toObject();

        query.addBindValue(obj["room"].toString());
        query.addBindValue(obj["course"].toString());
        query.addBindValue(obj["teacher"].toString());
        query.addBindValue(obj["time"].toString());
        query.addBindValue(obj["is_next"].toInt());

        if (!query.exec()) {
            qDebug() << "插入失败:" << query.lastError().text();
        }
    }

    if (db.commit()) {
        QString timeStr = QDateTime::currentDateTime().toString("HH:mm:ss");
        emit dataUpdated("同步成功 (Server): " + timeStr);
        qDebug() << "本地数据库已更新: " << array.size() << " 条记录";
    } else {
        db.rollback();
        qDebug() << "数据库写入失败";
    }
}
