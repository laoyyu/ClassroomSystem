#include "serverwindow.h"
#include <QVBoxLayout>
#include <QSqlQuery>
#include <QSqlError>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QDateTime>

ServerWindow::ServerWindow(QWidget *parent) : QWidget(parent)
{
    setWindowTitle("校园服务器 (Port: 12345)");
    resize(400, 300);

    logViewer = new QTextEdit(this);
    logViewer->setReadOnly(true);
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addWidget(logViewer);

    // 1. 初始化数据库
    initDb();

    // 2. 启动 TCP 监听
    tcpServer = new QTcpServer(this);
    connect(tcpServer, &QTcpServer::newConnection, this, &ServerWindow::onNewConnection);

    if (tcpServer->listen(QHostAddress::Any, 12345)) {
        logViewer->append("服务已启动，监听端口: 12345");
    } else {
        logViewer->append("服务启动失败: " + tcpServer->errorString());
    }
}

ServerWindow::~ServerWindow() {
    if(db.isOpen()) db.close();
}

void ServerWindow::initDb() {
    db = QSqlDatabase::addDatabase("QSQLITE", "ServerConnection");
    db.setDatabaseName("server_data.db");

    if (!db.open()) {
        logViewer->append("数据库打开失败");
        return;
    }

    // 创建表并插入初始数据（模拟教务系统排课）
    QSqlQuery query(db);
    query.exec("CREATE TABLE IF NOT EXISTS master_schedules ("
               "room TEXT, course TEXT, teacher TEXT, time_slot TEXT, is_next INTEGER)");

    // 每次重启先清空，写入模拟数据
    query.exec("DELETE FROM master_schedules");

    QString insertSql = "INSERT INTO master_schedules VALUES (?, ?, ?, ?, ?)";

    // 模拟数据 1：101教室当前课
    query.prepare(insertSql);
    query.addBindValue("Class 101");
    query.addBindValue("服务器分发的数学课");
    query.addBindValue("王教授(Server)");
    query.addBindValue(QDateTime::currentDateTime().toString("HH:mm") + " - " + QDateTime::currentDateTime().addSecs(3600).toString("HH:mm"));
    query.addBindValue(0);
    query.exec();

    // 模拟数据 2：101教室下节课
    query.prepare(insertSql);
    query.addBindValue("Class 101");
    query.addBindValue("服务器分发的英语课");
    query.addBindValue("李老师(Server)");
    query.addBindValue("14:00 - 15:30");
    query.addBindValue(1);
    query.exec();

    // 模拟数据 3：102教室
    query.prepare(insertSql);
    query.addBindValue("Class 102");
    query.addBindValue("物理实验");
    query.addBindValue("陈工");
    query.addBindValue("09:00 - 12:00");
    query.addBindValue(0);
    query.exec();

    logViewer->append("服务端数据库已重置并生成模拟数据。");
}

void ServerWindow::onNewConnection() {
    QTcpSocket *clientSocket = tcpServer->nextPendingConnection();
    connect(clientSocket, &QTcpSocket::readyRead, this, &ServerWindow::onReadClientData);
    connect(clientSocket, &QTcpSocket::disconnected, clientSocket, &QTcpSocket::deleteLater);

    logViewer->append("客户端已连接: " + clientSocket->peerAddress().toString());
}

void ServerWindow::onReadClientData() {
    QTcpSocket *socket = qobject_cast<QTcpSocket*>(sender());
    if (!socket) return;

    // 简单协议：只要客户端发来任何数据，我们就把整个课表吐给它
    QByteArray request = socket->readAll();
    logViewer->append("收到请求，正在发送数据...");

    QByteArray responseData = getScheduleJson();
    socket->write(responseData);
    socket->flush();
    // socket->disconnectFromHost(); // 发送完可以主动断开，也可以等客户端断开
}

QByteArray ServerWindow::getScheduleJson() {
    QSqlQuery query(db);
    query.exec("SELECT room, course, teacher, time_slot, is_next FROM master_schedules");

    QJsonArray jsonArray;
    while (query.next()) {
        QJsonObject obj;
        obj["room"] = query.value(0).toString();
        obj["course"] = query.value(1).toString();
        obj["teacher"] = query.value(2).toString();
        obj["time"] = query.value(3).toString();
        obj["is_next"] = query.value(4).toInt();
        jsonArray.append(obj);
    }

    QJsonDocument doc(jsonArray);
    return doc.toJson();
}
