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
#include <QDataStream>

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

    // 检查是否有数据，如果没有则自动初始化
    query.exec("SELECT COUNT(*) FROM master_schedules");
    if (query.next() && query.value(0).toInt() == 0) {
        logViewer->append("数据库为空，开始自动初始化示例数据...");
        initSampleData();
        
        // 再次检查
        query.exec("SELECT COUNT(*) FROM master_schedules");
        if (query.next()) {
            logViewer->append("初始化完成，课程表记录数: " + query.value(0).toString());
        }
    } else {
        logViewer->append("服务端数据库已连接，现有记录数: " + query.value(0).toString());
    }
}

void ServerWindow::initSampleData() {
    QSqlQuery query(db);
    QDateTime now = QDateTime::currentDateTime();
    QDateTime nextHour = now.addSecs(3600);
    QString currentTimeSlot = now.toString("HH:mm") + " - " + nextHour.toString("HH:mm");
    
    // 插入课程表数据（30条）
    QStringList schedules;
    schedules << QString("INSERT INTO master_schedules VALUES ('Class 101', '高等数学', '王教授', '%1', 0)").arg(currentTimeSlot);
    schedules << "INSERT INTO master_schedules VALUES ('Class 101', '大学英语', '李老师', '14:00 - 15:30', 1)";
    schedules << "INSERT INTO master_schedules VALUES ('Class 102', '大学物理', '陈工', '09:00 - 12:00', 0)";
    schedules << "INSERT INTO master_schedules VALUES ('Class 102', '线性代数', '张教授', '14:00 - 16:00', 1)";
    schedules << "INSERT INTO master_schedules VALUES ('Class 103', '计算机基础', '刘老师', '08:00 - 10:00', 0)";
    schedules << "INSERT INTO master_schedules VALUES ('Class 103', '数据结构', '赵教授', '10:30 - 12:30', 1)";
    schedules << "INSERT INTO master_schedules VALUES ('Class 104', '有机化学', '孙老师', '09:00 - 11:00', 0)";
    schedules << "INSERT INTO master_schedules VALUES ('Class 104', '分析化学', '周教授', '14:00 - 16:00', 1)";
    schedules << "INSERT INTO master_schedules VALUES ('Class 105', '大学语文', '吴老师', '08:30 - 10:30', 0)";
    schedules << "INSERT INTO master_schedules VALUES ('Class 105', '写作与表达', '郑老师', '11:00 - 12:30', 1)";
    schedules << "INSERT INTO master_schedules VALUES ('Class 201', '概率论', '冯教授', '09:00 - 11:00', 0)";
    schedules << "INSERT INTO master_schedules VALUES ('Class 201', '数理统计', '陈老师', '14:00 - 16:00', 1)";
    schedules << "INSERT INTO master_schedules VALUES ('Class 202', '电路原理', '杨教授', '08:00 - 10:00', 0)";
    schedules << "INSERT INTO master_schedules VALUES ('Class 202', '模拟电子技术', '朱老师', '10:30 - 12:30', 1)";
    schedules << "INSERT INTO master_schedules VALUES ('Class 203', '操作系统', '钱教授', '09:00 - 11:00', 0)";
    schedules << "INSERT INTO master_schedules VALUES ('Class 203', '计算机网络', '林老师', '14:00 - 16:00', 1)";
    schedules << "INSERT INTO master_schedules VALUES ('Class 204', '工程力学', '何教授', '08:30 - 10:30', 0)";
    schedules << "INSERT INTO master_schedules VALUES ('Class 204', '材料力学', '高老师', '11:00 - 12:30', 1)";
    schedules << "INSERT INTO master_schedules VALUES ('Class 205', '管理学原理', '马老师', '09:00 - 11:00', 0)";
    schedules << "INSERT INTO master_schedules VALUES ('Class 205', '市场营销', '罗老师', '14:00 - 16:00', 1)";
    schedules << "INSERT INTO master_schedules VALUES ('Class 301', '数据库系统', '梁教授', '08:00 - 10:00', 0)";
    schedules << "INSERT INTO master_schedules VALUES ('Class 301', '软件工程', '宋老师', '10:30 - 12:30', 1)";
    schedules << "INSERT INTO master_schedules VALUES ('Class 302', '人工智能导论', '唐教授', '09:00 - 11:00', 0)";
    schedules << "INSERT INTO master_schedules VALUES ('Class 302', '机器学习', '许老师', '14:00 - 16:00', 1)";
    schedules << "INSERT INTO master_schedules VALUES ('Class 303', '数字信号处理', '韩教授', '08:30 - 10:30', 0)";
    schedules << "INSERT INTO master_schedules VALUES ('Class 303', '通信原理', '邓老师', '11:00 - 12:30', 1)";
    schedules << "INSERT INTO master_schedules VALUES ('Class 304', '宏观经济学', '曹老师', '09:00 - 11:00', 0)";
    schedules << "INSERT INTO master_schedules VALUES ('Class 304', '微观经济学', '彭老师', '14:00 - 16:00', 1)";
    schedules << "INSERT INTO master_schedules VALUES ('Class 305', '法理学', '魏老师', '08:00 - 10:00', 0)";
    schedules << "INSERT INTO master_schedules VALUES ('Class 305', '宪法学', '谢老师', '10:30 - 12:30', 1)";
    
    for (const QString &sql : schedules) {
        if (!query.exec(sql)) {
            logViewer->append("插入课程失败: " + query.lastError().text());
        }
    }
    
    // 插入教室信息（15条）
    QStringList classrooms;
    classrooms << "INSERT INTO classrooms VALUES ('Class 101', '计算机科学2023级1班', 50, 'A栋', 3)";
    classrooms << "INSERT INTO classrooms VALUES ('Class 102', '计算机科学2023级2班', 45, 'A栋', 3)";
    classrooms << "INSERT INTO classrooms VALUES ('Class 103', '软件工程2023级1班', 60, 'B栋', 2)";
    classrooms << "INSERT INTO classrooms VALUES ('Class 104', '软件工程2023级2班', 55, 'B栋', 2)";
    classrooms << "INSERT INTO classrooms VALUES ('Class 105', '人工智能2023级1班', 40, 'C栋', 4)";
    classrooms << "INSERT INTO classrooms VALUES ('Class 201', '数据科学2023级1班', 50, 'A栋', 2)";
    classrooms << "INSERT INTO classrooms VALUES ('Class 202', '数据科学2023级2班', 45, 'A栋', 2)";
    classrooms << "INSERT INTO classrooms VALUES ('Class 203', '网络安奨2023级1班', 40, 'B栋', 3)";
    classrooms << "INSERT INTO classrooms VALUES ('Class 204', '网络安奨2023级2班', 40, 'B栋', 3)";
    classrooms << "INSERT INTO classrooms VALUES ('Class 205', '物联网2023级1班', 55, 'C栋', 2)";
    classrooms << "INSERT INTO classrooms VALUES ('Class 301', '计算机科学2024级1班', 50, 'A栋', 4)";
    classrooms << "INSERT INTO classrooms VALUES ('Class 302', '计算机科学2024级2班', 45, 'A栋', 4)";
    classrooms << "INSERT INTO classrooms VALUES ('Class 303', '软件工程2024级1班', 60, 'B栋', 1)";
    classrooms << "INSERT INTO classrooms VALUES ('Class 304', '软件工程2024级2班', 55, 'B栋', 1)";
    classrooms << "INSERT INTO classrooms VALUES ('Class 305', '人工智能2024级1班', 40, 'C栋', 3)";
    
    for (const QString &sql : classrooms) {
        if (!query.exec(sql)) {
            logViewer->append("插入教室失败: " + query.lastError().text());
        }
    }
    
    // 插入公告（3条）
    QString publishTime = now.toString("yyyy-MM-dd HH:mm:ss");
    QString expireTime1 = now.addDays(30).toString("yyyy-MM-dd HH:mm:ss");
    QString expireTime2 = now.addDays(7).toString("yyyy-MM-dd HH:mm:ss");
    QString expireTime3 = now.addDays(2).toString("yyyy-MM-dd HH:mm:ss");
    
    QStringList announcements;
    announcements << QString("INSERT INTO announcements VALUES ('系统通知', '欢迎使用智慧教室班牌系统！本系统提供课程信息查询、教室状态显示等功能。', 1, '%1', '%2')").arg(publishTime, expireTime1);
    announcements << QString("INSERT INTO announcements VALUES ('考试安排', '期末考试将于下周一开始，请同学们提前做好准备。', 2, '%1', '%2')").arg(publishTime, expireTime2);
    announcements << QString("INSERT INTO announcements VALUES ('维护通知', '系统将于本周六凌晨2:00-4:00进行维护升级，期间服务可能中断。', 0, '%1', '%2')").arg(publishTime, expireTime3);
    
    for (const QString &sql : announcements) {
        if (!query.exec(sql)) {
            logViewer->append("插入公告失败: " + query.lastError().text());
        }
    }
    
    logViewer->append("示例数据初始化完成：30条课程 + 15个教室 + 3条公告");
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

    logViewer->append("数据大小: " + QString::number(responseData.size()) + " 字节");

    QByteArray sizeData;
    QDataStream ds(&sizeData, QIODevice::WriteOnly);
    ds.setByteOrder(QDataStream::BigEndian);
    ds << static_cast<quint32>(responseData.size());

    qint64 bytesWritten = socket->write(sizeData);
    bytesWritten += socket->write(responseData);
    socket->flush();

    logViewer->append("已写入 " + QString::number(bytesWritten) + " 字节");

    if (socket->waitForBytesWritten(5000)) {
        logViewer->append("数据已完全发送，等待客户端断开连接...");
    } else {
        logViewer->append("数据发送超时");
    }
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
