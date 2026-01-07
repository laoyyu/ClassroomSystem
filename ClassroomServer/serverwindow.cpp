#include "serverwindow.h"
#include <QVBoxLayout>
#include <QSqlQuery>
#include <QSqlError>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QDateTime>
#include <QHostAddress>
#include <QObject>

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

    // 创建表（如果不存在）
    QSqlQuery query(db);
    query.exec("CREATE TABLE IF NOT EXISTS master_schedules ("
               "room TEXT, course TEXT, teacher TEXT, time_slot TEXT, is_next INTEGER)");

    query.exec("CREATE TABLE IF NOT EXISTS classrooms ("
               "room_name TEXT, class_name TEXT, capacity INTEGER, building TEXT, floor INTEGER)");

    query.exec("CREATE TABLE IF NOT EXISTS announcements ("
               "title TEXT, content TEXT, priority INTEGER, publish_time TEXT, expire_time TEXT)");

    // 检查是否有数据，如果没有则初始化
    query.exec("SELECT COUNT(*) FROM master_schedules");
    if (query.next() && query.value(0).toInt() == 0) {
        logViewer->append("数据库为空，请运行 init_db.py 初始化数据");
    } else {
        logViewer->append("服务端数据库已连接，现有记录数: " + query.value(0).toString());
    }
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

    QByteArray request = socket->readAll();
    logViewer->append("收到请求: " + request);
    logViewer->append("正在准备发送数据...");

    QByteArray responseData = getScheduleJson();
    responseData.append('\n');

    logViewer->append("数据大小: " + QString::number(responseData.size()) + " 字节");

    qint64 bytesWritten = socket->write(responseData);
    socket->flush();

    logViewer->append("已发送 " + QString::number(bytesWritten) + " 字节");

    socket->disconnectFromHost();
}

QByteArray ServerWindow::getScheduleJson() {
    QJsonObject rootObj;

    QJsonArray schedulesArray;
    QSqlQuery schedulesQuery(db);
    if (!schedulesQuery.exec("SELECT room, course, teacher, time_slot, is_next FROM master_schedules")) {
        logViewer->append("查询课程表失败: " + schedulesQuery.lastError().text());
    }
    while (schedulesQuery.next()) {
        QJsonObject obj;
        obj["room_name"] = schedulesQuery.value(0).toString();
        obj["course_name"] = schedulesQuery.value(1).toString();
        obj["teacher"] = schedulesQuery.value(2).toString();
        obj["time_slot"] = schedulesQuery.value(3).toString();
        obj["is_next"] = schedulesQuery.value(4).toInt();
        schedulesArray.append(obj);
    }
    logViewer->append("课程表记录数: " + QString::number(schedulesArray.size()));
    rootObj["schedules"] = schedulesArray;

    QJsonArray classroomsArray;
    QSqlQuery classroomsQuery(db);
    if (!classroomsQuery.exec("SELECT room_name, class_name, capacity, building, floor FROM classrooms")) {
        logViewer->append("查询教室信息失败: " + classroomsQuery.lastError().text());
    }
    while (classroomsQuery.next()) {
        QJsonObject obj;
        obj["room_name"] = classroomsQuery.value(0).toString();
        obj["class_name"] = classroomsQuery.value(1).toString();
        obj["capacity"] = classroomsQuery.value(2).toInt();
        obj["building"] = classroomsQuery.value(3).toString();
        obj["floor"] = classroomsQuery.value(4).toInt();
        classroomsArray.append(obj);
    }
    logViewer->append("教室信息记录数: " + QString::number(classroomsArray.size()));
    rootObj["classrooms"] = classroomsArray;

    QJsonArray announcementsArray;
    QSqlQuery announcementsQuery(db);
    if (!announcementsQuery.exec("SELECT title, content, priority, publish_time, expire_time FROM announcements")) {
        logViewer->append("查询公告失败: " + announcementsQuery.lastError().text());
    }
    while (announcementsQuery.next()) {
        QJsonObject obj;
        obj["title"] = announcementsQuery.value(0).toString();
        obj["content"] = announcementsQuery.value(1).toString();
        obj["priority"] = announcementsQuery.value(2).toInt();
        obj["publish_time"] = announcementsQuery.value(3).toString();
        obj["expire_time"] = announcementsQuery.value(4).toString();
        announcementsArray.append(obj);
    }
    logViewer->append("公告记录数: " + QString::number(announcementsArray.size()));
    rootObj["announcements"] = announcementsArray;

    QJsonDocument doc(rootObj);
    QByteArray jsonData = doc.toJson();
    logViewer->append("JSON数据预览: " + jsonData.left(100) + "...");
    
    return jsonData;
}
