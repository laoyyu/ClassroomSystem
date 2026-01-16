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
#include <QApplication>
#include <QHeaderView>
#include <QTableWidgetItem>
#include <QTimer>
#include <QFormLayout>
#include <QSpinBox>
#include <QLineEdit>
#include <QTextEdit>
#include <QAbstractItemView>
#include <QList>
#include <QMap>
#include <QVector>

ServerWindow::ServerWindow(QWidget *parent) : QWidget(parent)
{
    setWindowTitle("校园服务器 (Port: 12345)");
    resize(1200, 800);
    
    setupUi();
    
    // 1. 初始化数据库
    initDb();
    
    // 2. 刷新数据显示
    refreshData();
    
    // 3. 设置定时器更新当前上课班级信息（每分钟更新一次）
    classUpdateTimer = new QTimer(this);
    connect(classUpdateTimer, &QTimer::timeout, this, &ServerWindow::updateCurrentClasses);
    classUpdateTimer->start(60000); // 每分钟更新一次
    
    // 4. 启动 TCP 监听
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
               "id INTEGER PRIMARY KEY AUTOINCREMENT, "
               "room TEXT, course TEXT, teacher TEXT, time_slot TEXT, "
               "start_time TEXT, end_time TEXT, weekday INTEGER, is_next INTEGER)");

    // 检查并更新 classrooms 表结构
    query.exec("PRAGMA table_info(classrooms)");
    bool hasIdColumn = false;
    bool tableExists = query.next(); // 如果有结果，说明表存在
    
    if(tableExists) {
        // 遍历所有列信息，检查是否存在 id 列
        do {
            if(query.value(1).toString() == "id") {
                hasIdColumn = true;
                break;
            }
        } while(query.next());
        
        // 如果表存在但没有 id 列，则添加它
        if(!hasIdColumn) {
            logViewer->append("检测到旧版 classrooms 表，正在更新表结构...");
            
            // 重命名原表
            query.exec("ALTER TABLE classrooms RENAME TO classrooms_old");
            
            // 创建新表结构
            query.exec("CREATE TABLE classrooms ("
                       "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                       "room_name TEXT, class_name TEXT, capacity INTEGER, building TEXT, floor INTEGER, current_class TEXT)");
            
            // 从旧表复制数据到新表（跳过可能存在的 id 列）
            query.exec("INSERT INTO classrooms (room_name, class_name, capacity, building, floor, current_class) "
                       "SELECT room_name, class_name, capacity, building, floor, current_class FROM classrooms_old");
            
            // 删除旧表
            query.exec("DROP TABLE classrooms_old");
            
            logViewer->append("classrooms 表结构更新完成");
        }
    } else {
        // 如果表不存在，创建新表
        query.exec("CREATE TABLE IF NOT EXISTS classrooms ("
                   "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                   "room_name TEXT, class_name TEXT, capacity INTEGER, building TEXT, floor INTEGER, current_class TEXT)");
    }

    // 检查并更新 announcements 表结构
    query.exec("PRAGMA table_info(announcements)");
    bool hasAnnounceIdColumn = false;
    bool announceTableExists = query.next(); // 如果有结果，说明表存在
    
    if(announceTableExists) {
        // 遍历所有列信息，检查是否存在 id 列
        do {
            if(query.value(1).toString() == "id") {
                hasAnnounceIdColumn = true;
                break;
            }
        } while(query.next());
        
        // 如果表存在但没有 id 列，则添加它
        if(!hasAnnounceIdColumn) {
            logViewer->append("检测到旧版 announcements 表，正在更新表结构...");
            
            // 重命名原表
            query.exec("ALTER TABLE announcements RENAME TO announcements_old");
            
            // 创建新表结构
            query.exec("CREATE TABLE announcements (id INTEGER PRIMARY KEY AUTOINCREMENT, title TEXT, content TEXT, priority INTEGER, publish_time TEXT, expire_time TEXT)");
            
            // 从旧表复制数据到新表（跳过可能存在的 id 列）
            query.exec("INSERT INTO announcements (title, content, priority, publish_time, expire_time) SELECT title, content, priority, publish_time, expire_time FROM announcements_old");
            
            // 删除旧表
            query.exec("DROP TABLE announcements_old");
            
            logViewer->append("announcements 表结构更新完成");
        }
    } else {
        // 如果表不存在，创建新表
        query.exec("CREATE TABLE IF NOT EXISTS announcements (id INTEGER PRIMARY KEY AUTOINCREMENT, title TEXT, content TEXT, priority INTEGER, publish_time TEXT, expire_time TEXT)");
    }

    // 检查是否有数据，如果没有则自动初始化
    query.exec("SELECT COUNT(*) FROM master_schedules");
    int scheduleCount = 0;
    if (query.next()) {
        scheduleCount = query.value(0).toInt();
    }
    
    query.exec("SELECT COUNT(*) FROM classrooms");
    int classroomCount = 0;
    if (query.next()) {
        classroomCount = query.value(0).toInt();
    }
    
    if (scheduleCount == 0 || classroomCount == 0) {
        logViewer->append("数据库为空或数据不完整，开始自动初始化示例数据...");
        initSampleData();
        
        // 再次检查
        query.exec("SELECT COUNT(*) FROM master_schedules");
        if (query.next()) {
            scheduleCount = query.value(0).toInt();
            logViewer->append("初始化完成，课程表记录数: " + query.value(0).toString());
        }
        
        query.exec("SELECT COUNT(*) FROM classrooms");
        if (query.next()) {
            classroomCount = query.value(0).toInt();
            logViewer->append("初始化完成，教室信息记录数: " + query.value(0).toString());
        }
    } else {
        logViewer->append("服务端数据库已连接，课程表记录数: " + QString::number(scheduleCount) + ", 教室记录数: " + QString::number(classroomCount));
    }
    
    // 确保界面刷新显示最新数据
    if(classroomCount > 0) {
        populateClassroomsTable();
    }
    if(scheduleCount > 0) {
        populateSchedulesTable();
    }
}

void ServerWindow::setupUi() {
    // 创建主布局
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    
    // 创建标签页控件
    dataTabWidget = new QTabWidget();
    
    // 课程表页面
    QWidget *schedulesPage = new QWidget();
    QVBoxLayout *schedulesLayout = new QVBoxLayout(schedulesPage);
    schedulesTable = new QTableWidget();
    schedulesTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    schedulesTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    schedulesLayout->addWidget(schedulesTable);
    dataTabWidget->addTab(schedulesPage, "课程表");
    
    // 教室信息页面
    QWidget *classroomsPage = new QWidget();
    QVBoxLayout *classroomsLayout = new QVBoxLayout(classroomsPage);
    classroomsTable = new QTableWidget();
    classroomsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    classroomsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    classroomsLayout->addWidget(classroomsTable);
    dataTabWidget->addTab(classroomsPage, "教室信息");
    
    // 日志区域
    logViewer = new QTextEdit();
    logViewer->setReadOnly(true);
    logViewer->setMaximumHeight(200);
    
    // 公告页面
    QWidget *announcementsPage = new QWidget();
    QVBoxLayout *announcementsLayout = new QVBoxLayout(announcementsPage);
    announcementsTable = new QTableWidget();
    announcementsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    announcementsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    announcementsLayout->addWidget(announcementsTable);
    dataTabWidget->addTab(announcementsPage, "公告");
    
    // 管理页面
    QWidget *managementPage = new QWidget();
    QVBoxLayout *managementLayout = new QVBoxLayout(managementPage);
    setupManagementUi();
    managementLayout->addWidget(managementTabs);
    dataTabWidget->addTab(managementPage, "数据管理");
    
    // 刷新按钮
    refreshButton = new QPushButton("刷新数据");
    clearFilterButton = new QPushButton("清除筛选");
    weekDayFilterCombo = new QComboBox();
    weekDayFilterCombo->addItem("全部", -1); // -1 表示显示所有数据
    weekDayFilterCombo->addItem("星期一", 1);
    weekDayFilterCombo->addItem("星期二", 2);
    weekDayFilterCombo->addItem("星期三", 3);
    weekDayFilterCombo->addItem("星期四", 4);
    weekDayFilterCombo->addItem("星期五", 5);
    weekDayFilterCombo->addItem("星期六", 6);
    weekDayFilterCombo->addItem("星期日", 7);
    
    statusLabel = new QLabel("就绪");
    
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addWidget(new QLabel("筛选:"));
    buttonLayout->addWidget(weekDayFilterCombo);
    buttonLayout->addWidget(refreshButton);
    buttonLayout->addWidget(clearFilterButton);
    buttonLayout->addWidget(statusLabel);
    buttonLayout->addStretch();
    
    connect(refreshButton, &QPushButton::clicked, this, &ServerWindow::refreshData);
    connect(clearFilterButton, &QPushButton::clicked, this, [=]() {
        weekDayFilterCombo->setCurrentIndex(0); // 选择“全部”
        filterSchedulesByWeekday();
    });
    connect(weekDayFilterCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ServerWindow::onWeekDayFilterChanged);
    
    // 组装主布局
    mainLayout->addWidget(dataTabWidget);
    mainLayout->addLayout(buttonLayout);
    mainLayout->addWidget(logViewer);
}

void ServerWindow::initSampleData() {
    QSqlQuery query(db);
    
    // 定义一天的课程时间表（共 5 节课）
    struct TimeSlot {
        QString start;
        QString end;
        QString display;
    };
    
    QVector<TimeSlot> timeSlots = {
        {"08:00", "09:40", "08:00 - 09:40"},  // 第1节
        {"10:00", "11:40", "10:00 - 11:40"},  // 第2节
        {"13:30", "15:10", "13:30 - 15:10"},  // 第3节
        {"15:30", "17:10", "15:30 - 17:10"},  // 第4节
        {"19:00", "20:40", "19:00 - 20:40"}   // 第5节（晚上）
    };
    
    // 课程模板（为每个教室生成 5 节课）
    struct CourseTemplate {
        QString name;
        QString teacher;
    };
    
    QMap<QString, QVector<CourseTemplate>> coursesPerRoom;
    
    // Class 101 - 计算机科学2023级1班
    coursesPerRoom["Class 101"] = {
        {"高等数学", "王教授"},
        {"数据结构", "张老师"},
        {"大学英语", "李老师"},
        {"计算机组成原理", "刘教授"},
        {"编程实践", "赵老师"}
    };
    
    // Class 102 - 计算机科学2023级2班
    coursesPerRoom["Class 102"] = {
        {"大学物理", "陈工"},
        {"线性代数", "张教授"},
        {"离散数学", "周老师"},
        {"计算机网络", "吴教授"},
        {"数据库原理", "郑老师"}
    };
    
    // Class 103 - 软件工程2023级1班
    coursesPerRoom["Class 103"] = {
        {"计算机基础", "刘老师"},
        {"软件工程", "马教授"},
        {"操作系统", "宋老师"},
        {"软件测试", "唐教授"},
        {"项目管理", "韩老师"}
    };
    
    // Class 104 - 软件工程2023级2班
    coursesPerRoom["Class 104"] = {
        {"有机化学", "孙老师"},
        {"分析化学", "周教授"},
        {"物理化学", "曹老师"},
        {"化学实验", "彭教授"},
        {"环境化学", "魏老师"}
    };
    
    // Class 105 - 人工智能2023级1班
    coursesPerRoom["Class 105"] = {
        {"大学语文", "吴老师"},
        {"人工智能导论", "邓教授"},
        {"机器学习", "谢老师"},
        {"深度学习", "颜教授"},
        {"神经网络", "黑老师"}
    };
    
    // Class 201 - 数据科学2023级1班
    coursesPerRoom["Class 201"] = {
        {"概率论", "冯教授"},
        {"数理统计", "陈老师"},
        {"数据分析", "杨教授"},
        {"机器学习基础", "朱老师"},
        {"大数据技术", "陈教授"}
    };
    
    // Class 202 - 数据科学2023级2班
    coursesPerRoom["Class 202"] = {
        {"电路原理", "杨教授"},
        {"模拟电子技术", "朱老师"},
        {"数字电子技术", "钱教授"},
        {"信号与系统", "林老师"},
        {"嵌入式系统", "何教授"}
    };
    
    // Class 203 - 网络安夨2023级1班
    coursesPerRoom["Class 203"] = {
        {"操作系统", "钱教授"},
        {"计算机网络", "林老师"},
        {"网络安全", "胡教授"},
        {"密码学", "谢老师"},
        {"网络攻防", "郭教授"}
    };
    
    // Class 204 - 网络安夨2023级2班
    coursesPerRoom["Class 204"] = {
        {"工程力学", "何教授"},
        {"材料力学", "高老师"},
        {"结构力学", "梁教授"},
        {"流体力学", "宋老师"},
        {"理论力学", "唐教授"}
    };
    
    // Class 205 - 物联网2023级1班
    coursesPerRoom["Class 205"] = {
        {"管理学原理", "马老师"},
        {"市场营销", "罗老师"},
        {"组织行为学", "许教授"},
        {"人力资源", "韩老师"},
        {"战略管理", "郓教授"}
    };
    
    // Class 301 - 计算机科学2024级1班
    coursesPerRoom["Class 301"] = {
        {"数据库系统", "梁教授"},
        {"软件工程", "宋老师"},
        {"编译原理", "唐教授"},
        {"算法设计", "许老师"},
        {"计算机图形学", "韩教授"}
    };
    
    // Class 302 - 计算机科学2024级2班
    coursesPerRoom["Class 302"] = {
        {"人工智能导论", "唐教授"},
        {"机器学习", "许老师"},
        {"计算机视觉", "韩教授"},
        {"自然语言处理", "郓老师"},
        {"智能系统", "曹教授"}
    };
    
    // Class 303 - 软件工程2024级1班
    coursesPerRoom["Class 303"] = {
        {"数字信号处理", "韩教授"},
        {"通信原理", "邓老师"},
        {"信息论", "曹教授"},
        {"移动通信", "彭老师"},
        {"无线网络", "魏教授"}
    };
    
    // Class 304 - 软件工程2024级2班
    coursesPerRoom["Class 304"] = {
        {"宏观经济学", "曹老师"},
        {"微观经济学", "彭老师"},
        {"计量经济学", "魏教授"},
        {"金融学", "谢老师"},
        {"国际经济学", "颜教授"}
    };
    
    // Class 305 - 人工智能2024级1班
    coursesPerRoom["Class 305"] = {
        {"法理学", "魏老师"},
        {"宪法学", "谢老师"},
        {"民法学", "颜教授"},
        {"刑法学", "黑老师"},
        {"行政法学", "白教授"}
    };
    
    // 仅为工作日生成课程记录（星期一到星期五，即1到5）
    for (int weekday = 1; weekday <= 5; ++weekday) {
        for (auto it = coursesPerRoom.constBegin(); it != coursesPerRoom.constEnd(); ++it) {
            QString roomName = it.key();
            QVector<CourseTemplate> courses = it.value();
            
            for (int i = 0; i < qMin(courses.size(), timeSlots.size()); ++i) {
                QString sql = QString(
                    "INSERT INTO master_schedules "
                    "(room, course, teacher, time_slot, start_time, end_time, weekday, is_next) "
                    "VALUES ('%1', '%2', '%3', '%4', '%5', '%6', %7, 0)"
                ).arg(
                    roomName,
                    courses[i].name,
                    courses[i].teacher,
                    timeSlots[i].display,
                    timeSlots[i].start,
                    timeSlots[i].end,
                    QString::number(weekday)
                );
                
                if (!query.exec(sql)) {
                    logViewer->append("插入课程失败: " + query.lastError().text());
                }
            }
        }
    }
    
    logViewer->append(QString("已生成 %1 个教室的课程表，每天 5 节课，共一周 5 个工作日").arg(coursesPerRoom.size()));
    
    // 插入教室信息（15条）
    QStringList classrooms;
    classrooms << "INSERT INTO classrooms (room_name, class_name, capacity, building, floor, current_class) VALUES ('Class 101', '计算机科学2023级1班', 50, 'A栋', 3, '')";
    classrooms << "INSERT INTO classrooms (room_name, class_name, capacity, building, floor, current_class) VALUES ('Class 102', '计算机科学2023级2班', 45, 'A栋', 3, '')";
    classrooms << "INSERT INTO classrooms (room_name, class_name, capacity, building, floor, current_class) VALUES ('Class 103', '软件工程2023级1班', 60, 'B栋', 2, '')";
    classrooms << "INSERT INTO classrooms (room_name, class_name, capacity, building, floor, current_class) VALUES ('Class 104', '软件工程2023级2班', 55, 'B栋', 2, '')";
    classrooms << "INSERT INTO classrooms (room_name, class_name, capacity, building, floor, current_class) VALUES ('Class 105', '人工智能2023级1班', 40, 'C栋', 4, '')";
    classrooms << "INSERT INTO classrooms (room_name, class_name, capacity, building, floor, current_class) VALUES ('Class 201', '数据科学2023级1班', 50, 'A栋', 2, '')";
    classrooms << "INSERT INTO classrooms (room_name, class_name, capacity, building, floor, current_class) VALUES ('Class 202', '数据科学2023级2班', 45, 'A栋', 2, '')";
    classrooms << "INSERT INTO classrooms (room_name, class_name, capacity, building, floor, current_class) VALUES ('Class 203', '网络安奨2023级1班', 40, 'B栋', 3, '')";
    classrooms << "INSERT INTO classrooms (room_name, class_name, capacity, building, floor, current_class) VALUES ('Class 204', '网络安奨2023级2班', 40, 'B栋', 3, '')";
    classrooms << "INSERT INTO classrooms (room_name, class_name, capacity, building, floor, current_class) VALUES ('Class 205', '物联网2023级1班', 55, 'C栋', 2, '')";
    classrooms << "INSERT INTO classrooms (room_name, class_name, capacity, building, floor, current_class) VALUES ('Class 301', '计算机科学2024级1班', 50, 'A栋', 4, '')";
    classrooms << "INSERT INTO classrooms (room_name, class_name, capacity, building, floor, current_class) VALUES ('Class 302', '计算机科学2024级2班', 45, 'A栋', 4, '')";
    classrooms << "INSERT INTO classrooms (room_name, class_name, capacity, building, floor, current_class) VALUES ('Class 303', '软件工程2024级1班', 60, 'B栋', 1, '')";
    classrooms << "INSERT INTO classrooms (room_name, class_name, capacity, building, floor, current_class) VALUES ('Class 304', '软件工程2024级2班', 55, 'B栋', 1, '')";
    classrooms << "INSERT INTO classrooms (room_name, class_name, capacity, building, floor, current_class) VALUES ('Class 305', '人工智能2024级1班', 40, 'C栋', 3, '')";
    
    for (const QString &sql : classrooms) {
        if (!query.exec(sql)) {
            logViewer->append("插入教室失败: " + query.lastError().text());
        }
    }
    
    // 插入公告（3条）
    QDateTime now = QDateTime::currentDateTime();
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
    
    // 初始化后更新当前上课班级信息
    updateCurrentClasses();
}

void ServerWindow::updateCurrentClasses() {
    // 清空当前班级信息
    QSqlQuery clearQuery(db);
    clearQuery.exec("UPDATE classrooms SET current_class = ''");
    
    // 根据当前时间获取正在上课的课程
    QTime currentTime = QTime::currentTime();
    QDate currentDate = QDate::currentDate();
    int currentWeekday = currentDate.dayOfWeek(); // 1=Monday, 7=Sunday
    
    QSqlQuery query(db);
    QString sql = QString(
        "SELECT room, course, teacher FROM master_schedules WHERE weekday = %1 AND "
        "time(start_time) <= time('%2') AND time(end_time) >= time('%2')")
        .arg(currentWeekday)
        .arg(currentTime.toString("hh:mm:ss"));
    
    if (query.exec(sql)) {
        while (query.next()) {
            QString roomName = query.value(0).toString();
            QString courseName = query.value(1).toString();
            QString teacherName = query.value(2).toString();
            
            // 更新教室的当前班级信息
            QSqlQuery updateQuery(db);
            QString updateSql = QString(
                "UPDATE classrooms SET current_class = '%1' WHERE room_name = '%2'")
                .arg(courseName + " (" + teacherName + ")")
                .arg(roomName);
            updateQuery.exec(updateSql);
            
            logViewer->append(QString("教室 %1 正在上课: %2").arg(roomName, courseName));
        }
    }
}

void ServerWindow::refreshData() {
    if(!db.isOpen()) {
        logViewer->append("数据库未打开，无法刷新数据");
        statusLabel->setText("数据库错误");
        return;
    }
    
    statusLabel->setText("正在刷新数据...");
    QApplication::processEvents(); // 确保UI更新
    
    populateSchedulesTable();
    populateClassroomsTable();
    populateAnnouncementsTable();
    
    statusLabel->setText("数据刷新完成");
}

void ServerWindow::populateSchedulesTable() {
    // 默认显示全部数据
    weekDayFilterCombo->setCurrentIndex(0); // 选择“全部”
    filterSchedulesByWeekday();
}

void ServerWindow::populateClassroomsTable() {
    QSqlQuery query(db);
    if (!query.exec("SELECT room_name, class_name, capacity, building, floor, current_class FROM classrooms ORDER BY room_name")) {
        logViewer->append("查询教室信息失败: " + query.lastError().text());
        return;
    }
    
    // 获取行数
    int rowCount = 0;
    QSqlQuery countQuery(db);
    countQuery.exec("SELECT COUNT(*) FROM classrooms");
    if (countQuery.next()) {
        rowCount = countQuery.value(0).toInt();
    }
    
    classroomsTable->setRowCount(rowCount);
    classroomsTable->setColumnCount(6);
    classroomsTable->setHorizontalHeaderLabels({"教室名称", "班级名称", "容量", "楼栋", "楼层", "当前班级"});
    
    int row = 0;
    while (query.next()) {
        classroomsTable->setItem(row, 0, new QTableWidgetItem(query.value(0).toString()));
        classroomsTable->setItem(row, 1, new QTableWidgetItem(query.value(1).toString()));
        classroomsTable->setItem(row, 2, new QTableWidgetItem(QString::number(query.value(2).toInt())));
        classroomsTable->setItem(row, 3, new QTableWidgetItem(query.value(3).toString()));
        classroomsTable->setItem(row, 4, new QTableWidgetItem(QString::number(query.value(4).toInt())));
        classroomsTable->setItem(row, 5, new QTableWidgetItem(query.value(5).toString()));
        row++;
    }
    
    // 调整列宽
    classroomsTable->resizeColumnsToContents();
    logViewer->append("教室信息数据已加载: " + QString::number(rowCount) + " 条记录");
}

void ServerWindow::populateAnnouncementsTable() {
    QSqlQuery query(db);
    if (!query.exec("SELECT title, content, priority, publish_time, expire_time FROM announcements ORDER BY priority DESC, publish_time DESC")) {
        logViewer->append("查询公告失败: " + query.lastError().text());
        return;
    }
    
    // 获取行数
    int rowCount = 0;
    QSqlQuery countQuery(db);
    countQuery.exec("SELECT COUNT(*) FROM announcements");
    if (countQuery.next()) {
        rowCount = countQuery.value(0).toInt();
    }
    
    announcementsTable->setRowCount(rowCount);
    announcementsTable->setColumnCount(5);
    announcementsTable->setHorizontalHeaderLabels({"标题", "内容", "优先级", "发布时间", "过期时间"});
    
    int row = 0;
    while (query.next()) {
        announcementsTable->setItem(row, 0, new QTableWidgetItem(query.value(0).toString()));
        QTableWidgetItem *contentItem = new QTableWidgetItem(query.value(1).toString());
        contentItem->setToolTip(query.value(1).toString()); // 设置工具提示以显示完整内容
        announcementsTable->setItem(row, 1, contentItem);
        announcementsTable->setItem(row, 2, new QTableWidgetItem(QString::number(query.value(2).toInt())));
        announcementsTable->setItem(row, 3, new QTableWidgetItem(query.value(3).toString()));
        announcementsTable->setItem(row, 4, new QTableWidgetItem(query.value(4).toString()));
        row++;
    }
    
    // 调整列宽
    announcementsTable->resizeColumnsToContents();
    // 对于内容列限制宽度并允许换行显示
    announcementsTable->setColumnWidth(1, 300);
    logViewer->append("公告数据已加载: " + QString::number(rowCount) + " 条记录");
}

void ServerWindow::onWeekDayFilterChanged() {
    filterSchedulesByWeekday();
}

void ServerWindow::filterSchedulesByWeekday() {
    int selectedWeekDay = weekDayFilterCombo->currentData().toInt();
    
    QSqlQuery query(db);
    QString sql = "SELECT room, course, teacher, time_slot, start_time, end_time, weekday, is_next FROM master_schedules ";
    
    if (selectedWeekDay != -1) { // -1 表示显示全部
        sql += QString("WHERE weekday = %1 ").arg(selectedWeekDay);
    }
    sql += "ORDER BY room, weekday, start_time";
    
    if (!query.exec(sql)) {
        logViewer->append("筛选课程表失败: " + query.lastError().text());
        return;
    }
    
    // 获取行数
    int rowCount = 0;
    QSqlQuery countQuery(db);
    QString countSql = "SELECT COUNT(*) FROM master_schedules ";
    if (selectedWeekDay != -1) {
        countSql += QString("WHERE weekday = %1").arg(selectedWeekDay);
    }
    
    if (countQuery.exec(countSql)) {
        if (countQuery.next()) {
            rowCount = countQuery.value(0).toInt();
        }
    }
    
    schedulesTable->setRowCount(rowCount);
    schedulesTable->setColumnCount(8);
    schedulesTable->setHorizontalHeaderLabels({"教室", "课程", "教师", "时间段", "开始时间", "结束时间", "星期", "是否下一节"});
    
    int row = 0;
    while (query.next()) {
        schedulesTable->setItem(row, 0, new QTableWidgetItem(query.value(0).toString()));
        schedulesTable->setItem(row, 1, new QTableWidgetItem(query.value(1).toString()));
        schedulesTable->setItem(row, 2, new QTableWidgetItem(query.value(2).toString()));
        schedulesTable->setItem(row, 3, new QTableWidgetItem(query.value(3).toString()));
        schedulesTable->setItem(row, 4, new QTableWidgetItem(query.value(4).toString()));
        schedulesTable->setItem(row, 5, new QTableWidgetItem(query.value(5).toString()));
        schedulesTable->setItem(row, 6, new QTableWidgetItem(QString::number(query.value(6).toInt())));
        schedulesTable->setItem(row, 7, new QTableWidgetItem(QString::number(query.value(7).toInt())));
        row++;
    }
    
    // 调整列宽
    schedulesTable->resizeColumnsToContents();
    
    if (selectedWeekDay == -1) {
        logViewer->append("课程表数据已加载: " + QString::number(rowCount) + " 条记录（全部星期）");
    } else {
        QString weekDayStr;
        switch (selectedWeekDay) {
        case 1: weekDayStr = "星期一"; break;
        case 2: weekDayStr = "星期二"; break;
        case 3: weekDayStr = "星期三"; break;
        case 4: weekDayStr = "星期四"; break;
        case 5: weekDayStr = "星期五"; break;
        case 6: weekDayStr = "星期六"; break;
        case 7: weekDayStr = "星期日"; break;
        default: weekDayStr = QString::number(selectedWeekDay); break;
        }
        logViewer->append("课程表数据已加载: " + QString::number(rowCount) + " 条记录（" + weekDayStr + "）");
    }
}

void ServerWindow::onNewConnection() {
    QTcpSocket *clientSocket = tcpServer->nextPendingConnection();
    connect(clientSocket, &QTcpSocket::readyRead, this, &ServerWindow::onReadClientData);
    connect(clientSocket, &QTcpSocket::disconnected, this, &ServerWindow::onClientDisconnected);
    connect(clientSocket, &QTcpSocket::disconnected, clientSocket, &QTcpSocket::deleteLater);

    // 将socket存储起来，便于后续管理和清理
    clientSockets.insert(clientSocket);
    
    logViewer->append("客户端已连接: " + clientSocket->peerAddress().toString());
}

void ServerWindow::onReadClientData() {
    QTcpSocket *socket = qobject_cast<QTcpSocket*>(sender());
    if (!socket) return;

    // 检查是否有数据可读
    if (socket->bytesAvailable() == 0) {
        return; // 如果没有数据可读，则直接返回
    }
    
    // 读取数据
    QByteArray requestData = socket->readAll();
    QString requestStr = QString::fromUtf8(requestData);
    logViewer->append("收到请求: " + requestStr);
    
    // 验证请求内容，只有特定请求才返回数据
    if (requestStr.trimmed().isEmpty() || requestStr.contains("GET_SCHEDULE", Qt::CaseInsensitive) || requestStr.contains("SYNC", Qt::CaseInsensitive)) {
        logViewer->append("正在准备发送数据...");

        QByteArray responseData = getScheduleJson();

        logViewer->append("数据大小: " + QString::number(responseData.size()) + " 字节");

        // 先发送数据大小（4字节，大端序），再发送实际数据
        QByteArray sizeData;
        QDataStream ds(&sizeData, QIODevice::WriteOnly);
        ds.setByteOrder(QDataStream::BigEndian);
        ds << static_cast<quint32>(responseData.size());

        // 发送数据大小信息
        qint64 sizeBytesWritten = socket->write(sizeData);
        if (sizeBytesWritten != 4) {
            logViewer->append("发送数据大小信息失败");
            return;
        }

        // 发送实际数据
        qint64 dataBytesWritten = socket->write(responseData);
        if (dataBytesWritten != responseData.size()) {
            logViewer->append("发送数据失败，期望发送" + QString::number(responseData.size()) + "字节，实际发送" + QString::number(dataBytesWritten) + "字节");
            return;
        }
        
        socket->flush();

        qint64 totalBytesWritten = sizeBytesWritten + dataBytesWritten;
        logViewer->append("已写入 " + QString::number(totalBytesWritten) + " 字节");

        if (socket->waitForBytesWritten(5000)) {
            logViewer->append("数据已完全发送，等待客户端断开连接...");
        } else {
            logViewer->append("数据发送超时");
        }
    } else {
        logViewer->append("无效请求: " + requestStr + ", 拒绝发送数据");
        // 对无效请求立即断开连接以防止滥用
        socket->disconnectFromHost();
    }
}

QByteArray ServerWindow::getScheduleJson() {
    if(!db.isOpen()) {
        logViewer->append("数据库未打开，无法获取数据");
        return QJsonDocument(QJsonObject()).toJson();
    }
    
    QJsonObject rootObj;

    QJsonArray schedulesArray;
    QSqlQuery schedulesQuery(db);
    if (!schedulesQuery.exec("SELECT room, course, teacher, time_slot, start_time, end_time, weekday, is_next FROM master_schedules")) {
        logViewer->append("查询课程表失败: " + schedulesQuery.lastError().text());
        return QJsonDocument(QJsonObject()).toJson(); // 返回空JSON
    }
    while (schedulesQuery.next()) {
        QJsonObject obj;
        obj["room_name"] = schedulesQuery.value(0).toString();
        obj["course_name"] = schedulesQuery.value(1).toString();
        obj["teacher"] = schedulesQuery.value(2).toString();
        obj["time_slot"] = schedulesQuery.value(3).toString();
        obj["start_time"] = schedulesQuery.value(4).toString();
        obj["end_time"] = schedulesQuery.value(5).toString();
        obj["weekday"] = schedulesQuery.value(6).toInt();
        obj["is_next"] = schedulesQuery.value(7).toInt();
        schedulesArray.append(obj);
    }
    logViewer->append("课程表记录数: " + QString::number(schedulesArray.size()));
    rootObj["schedules"] = schedulesArray;

    QJsonArray classroomsArray;
    QSqlQuery classroomsQuery(db);
    if (!classroomsQuery.exec("SELECT room_name, class_name, capacity, building, floor, current_class FROM classrooms")) {
        logViewer->append("查询教室信息失败: " + classroomsQuery.lastError().text());
        return QJsonDocument(QJsonObject()).toJson(); // 返回空JSON
    }
    while (classroomsQuery.next()) {
        QJsonObject obj;
        obj["room_name"] = classroomsQuery.value(0).toString();
        obj["class_name"] = classroomsQuery.value(1).toString();
        obj["capacity"] = classroomsQuery.value(2).toInt();
        obj["building"] = classroomsQuery.value(3).toString();
        obj["floor"] = classroomsQuery.value(4).toInt();
        obj["current_class"] = classroomsQuery.value(5).toString();
        classroomsArray.append(obj);
    }
    logViewer->append("教室信息记录数: " + QString::number(classroomsArray.size()));
    rootObj["classrooms"] = classroomsArray;

    QJsonArray announcementsArray;
    QSqlQuery announcementsQuery(db);
    if (!announcementsQuery.exec("SELECT title, content, priority, publish_time, expire_time FROM announcements")) {
        logViewer->append("查询公告失败: " + announcementsQuery.lastError().text());
        return QJsonDocument(QJsonObject()).toJson(); // 返回空JSON
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

void ServerWindow::onClientDisconnected() {
    QTcpSocket *socket = qobject_cast<QTcpSocket*>(sender());
    if (socket && clientSockets.contains(socket)) {
        clientSockets.remove(socket);
        logViewer->append("客户端已断开连接: " + socket->peerAddress().toString());
    }
}

// CRUD 操作实现
bool ServerWindow::addCourse(const QString& room, const QString& course, const QString& teacher,
                             const QString& timeSlot, const QString& startTime, const QString& endTime,
                             int weekday, int isNext) {
    if (!db.isOpen()) {
        logViewer->append("数据库未打开，无法添加课程");
        return false;
    }
    
    // 验证必要字段不为空
    if (room.isEmpty() || course.isEmpty() || teacher.isEmpty()) {
        logViewer->append("教室、课程和教师不能为空");
        return false;
    }
    
    QSqlQuery query(db);
    query.prepare("INSERT INTO master_schedules (room, course, teacher, time_slot, start_time, end_time, weekday, is_next) "
               "VALUES (?, ?, ?, ?, ?, ?, ?, ?)");
    query.addBindValue(room);
    query.addBindValue(course);
    query.addBindValue(teacher);
    query.addBindValue(timeSlot);
    query.addBindValue(startTime);
    query.addBindValue(endTime);
    query.addBindValue(weekday);
    query.addBindValue(isNext);
    
    if (!query.exec()) {
        logViewer->append("添加课程失败: " + query.lastError().text());
        return false;
    }
    
    logViewer->append(QString("课程添加成功: %1 - %2 (%3)").arg(room, course, teacher));
    refreshData(); // 刷新界面显示
    return true;
}

bool ServerWindow::updateCourse(int id, const QString& room, const QString& course, const QString& teacher,
                               const QString& timeSlot, const QString& startTime, const QString& endTime,
                               int weekday, int isNext) {
    if (!db.isOpen()) {
        logViewer->append("数据库未打开，无法更新课程");
        return false;
    }
    
    // 验证必要字段不为空
    if (room.isEmpty() || course.isEmpty() || teacher.isEmpty()) {
        logViewer->append("教室、课程和教师不能为空");
        return false;
    }
    
    QSqlQuery query(db);
    query.prepare("UPDATE master_schedules SET room=?, course=?, teacher=?, time_slot=?, start_time=?, "
               "end_time=?, weekday=?, is_next=? WHERE id=?");
    query.addBindValue(room);
    query.addBindValue(course);
    query.addBindValue(teacher);
    query.addBindValue(timeSlot);
    query.addBindValue(startTime);
    query.addBindValue(endTime);
    query.addBindValue(weekday);
    query.addBindValue(isNext);
    query.addBindValue(id);
    
    if (!query.exec()) {
        logViewer->append("更新课程失败: " + query.lastError().text());
        return false;
    }
    
    if (query.numRowsAffected() > 0) {
        logViewer->append(QString("课程更新成功: ID=%1").arg(id));
        refreshData(); // 刷新界面显示
        return true;
    } else {
        logViewer->append(QString("未找到要更新的课程: ID=%1").arg(id));
        return false;
    }
}

bool ServerWindow::deleteCourse(int id) {
    if (!db.isOpen()) {
        logViewer->append("数据库未打开，无法删除课程");
        return false;
    }
    
    QSqlQuery query(db);
    query.prepare("DELETE FROM master_schedules WHERE id = ?");
    query.addBindValue(id);
    
    if (!query.exec()) {
        logViewer->append("删除课程失败: " + query.lastError().text());
        return false;
    }
    
    if (query.numRowsAffected() > 0) {
        logViewer->append(QString("课程删除成功: ID=%1").arg(id));
        refreshData(); // 刷新界面显示
        return true;
    } else {
        logViewer->append(QString("未找到要删除的课程: ID=%1").arg(id));
        return false;
    }
}

bool ServerWindow::addClassroom(const QString& roomName, const QString& className, int capacity,
                              const QString& building, int floor, const QString& currentClass) {
    if (!db.isOpen()) {
        logViewer->append("数据库未打开，无法添加教室");
        return false;
    }
    
    // 验证必要字段不为空
    if (roomName.isEmpty() || className.isEmpty()) {
        logViewer->append("教室名称和班级名称不能为空");
        return false;
    }
    
    // 检查教室名称是否已存在
    QSqlQuery checkQuery(db);
    checkQuery.prepare("SELECT COUNT(*) FROM classrooms WHERE room_name = ?");
    checkQuery.addBindValue(roomName);
    if (checkQuery.exec() && checkQuery.next()) {
        if (checkQuery.value(0).toInt() > 0) {
            logViewer->append("教室名称已存在: " + roomName);
            return false;
        }
    }
    
    QSqlQuery query(db);
    query.prepare("INSERT INTO classrooms (room_name, class_name, capacity, building, floor, current_class) "
               "VALUES (?, ?, ?, ?, ?, ?)");
    query.addBindValue(roomName);
    query.addBindValue(className);
    query.addBindValue(capacity);
    query.addBindValue(building);
    query.addBindValue(floor);
    query.addBindValue(currentClass);
    
    if (!query.exec()) {
        logViewer->append("添加教室失败: " + query.lastError().text());
        return false;
    }
    
    logViewer->append(QString("教室添加成功: %1 - %2").arg(roomName, className));
    refreshData(); // 刷新界面显示
    return true;
}

bool ServerWindow::updateClassroom(const QString& roomName, const QString& className, int capacity,
                                 const QString& building, int floor, const QString& currentClass) {
    if (!db.isOpen()) {
        logViewer->append("数据库未打开，无法更新教室");
        return false;
    }
    
    // 验证必要字段不为空
    if (roomName.isEmpty() || className.isEmpty()) {
        logViewer->append("教室名称和班级名称不能为空");
        return false;
    }
    
    QSqlQuery query(db);
    query.prepare("UPDATE classrooms SET class_name=?, capacity=?, building=?, floor=?, current_class=? WHERE room_name=?");
    query.addBindValue(className);
    query.addBindValue(capacity);
    query.addBindValue(building);
    query.addBindValue(floor);
    query.addBindValue(currentClass);
    query.addBindValue(roomName);
    
    if (!query.exec()) {
        logViewer->append("更新教室失败: " + query.lastError().text());
        return false;
    }
    
    if (query.numRowsAffected() > 0) {
        logViewer->append(QString("教室更新成功: %1").arg(roomName));
        refreshData(); // 刷新界面显示
        return true;
    } else {
        logViewer->append(QString("未找到要更新的教室: %1").arg(roomName));
        return false;
    }
}

bool ServerWindow::deleteClassroom(const QString& roomName) {
    if (!db.isOpen()) {
        logViewer->append("数据库未打开，无法删除教室");
        return false;
    }
    
    // 验证教室名称不为空
    if (roomName.isEmpty()) {
        logViewer->append("教室名称不能为空");
        return false;
    }
    
    QSqlQuery query(db);
    query.prepare("DELETE FROM classrooms WHERE room_name = ?");
    query.addBindValue(roomName);
    
    if (!query.exec()) {
        logViewer->append("删除教室失败: " + query.lastError().text());
        return false;
    }
    
    if (query.numRowsAffected() > 0) {
        logViewer->append(QString("教室删除成功: %1").arg(roomName));
        refreshData(); // 刷新界面显示
        return true;
    } else {
        logViewer->append(QString("未找到要删除的教室: %1").arg(roomName));
        return false;
    }
}

bool ServerWindow::addAnnouncement(const QString& title, const QString& content, int priority,
                                 const QString& publishTime, const QString& expireTime) {
    if (!db.isOpen()) {
        logViewer->append("数据库未打开，无法添加公告");
        return false;
    }
    
    // 验证必要字段不为空
    if (title.isEmpty() || content.isEmpty()) {
        logViewer->append("公告标题和内容不能为空");
        return false;
    }
    
    QSqlQuery query(db);
    query.prepare("INSERT INTO announcements (title, content, priority, publish_time, expire_time) "
               "VALUES (?, ?, ?, ?, ?)");
    query.addBindValue(title);
    query.addBindValue(content);
    query.addBindValue(priority);
    query.addBindValue(publishTime);
    query.addBindValue(expireTime);
    
    if (!query.exec()) {
        logViewer->append("添加公告失败: " + query.lastError().text());
        return false;
    }
    
    logViewer->append(QString("公告添加成功: %1").arg(title));
    refreshData(); // 刷新界面显示
    return true;
}

bool ServerWindow::updateAnnouncement(int id, const QString& title, const QString& content, int priority,
                                    const QString& publishTime, const QString& expireTime) {
    if (!db.isOpen()) {
        logViewer->append("数据库未打开，无法更新公告");
        return false;
    }
    
    // 验证必要字段不为空
    if (title.isEmpty() || content.isEmpty()) {
        logViewer->append("公告标题和内容不能为空");
        return false;
    }
    
    QSqlQuery query(db);
    query.prepare("UPDATE announcements SET title=?, content=?, priority=?, publish_time=?, expire_time=? WHERE id=?");
    query.addBindValue(title);
    query.addBindValue(content);
    query.addBindValue(priority);
    query.addBindValue(publishTime);
    query.addBindValue(expireTime);
    query.addBindValue(id);
    
    if (!query.exec()) {
        logViewer->append("更新公告失败: " + query.lastError().text());
        return false;
    }
    
    if (query.numRowsAffected() > 0) {
        logViewer->append(QString("公告更新成功: ID=%1").arg(id));
        refreshData(); // 刷新界面显示
        return true;
    } else {
        logViewer->append(QString("未找到要更新的公告: ID=%1").arg(id));
        return false;
    }
}

bool ServerWindow::deleteAnnouncement(int id) {
    if (!db.isOpen()) {
        logViewer->append("数据库未打开，无法删除公告");
        return false;
    }
    
    QSqlQuery query(db);
    query.prepare("DELETE FROM announcements WHERE id = ?");
    query.addBindValue(id);
    
    if (!query.exec()) {
        logViewer->append("删除公告失败: " + query.lastError().text());
        return false;
    }
    
    if (query.numRowsAffected() > 0) {
        logViewer->append(QString("公告删除成功: ID=%1").arg(id));
        refreshData(); // 刷新界面显示
        return true;
    } else {
        logViewer->append(QString("未找到要删除的公告: ID=%1").arg(id));
        return false;
    }
}

// 管理界面函数实现
void ServerWindow::setupManagementUi() {
    // 创建管理标签页
    managementTabs = new QTabWidget();
    
    // 创建课程管理页面
    setupCourseManagementPage();
    
    // 创建教室管理页面
    setupClassroomManagementPage();
    
    // 创建公告管理页面
    setupAnnouncementManagementPage();
}

void ServerWindow::setupCourseManagementPage() {
    courseManagementPage = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(courseManagementPage);
    
    // 输入表单
    QFormLayout *formLayout = new QFormLayout();
    
    roomLineEdit = new QLineEdit();
    formLayout->addRow("教室:", roomLineEdit);
    
    courseLineEdit = new QLineEdit();
    formLayout->addRow("课程:", courseLineEdit);
    
    teacherLineEdit = new QLineEdit();
    formLayout->addRow("教师:", teacherLineEdit);
    
    timeSlotLineEdit = new QLineEdit();
    formLayout->addRow("时间段:", timeSlotLineEdit);
    
    startTimeLineEdit = new QLineEdit();
    formLayout->addRow("开始时间:", startTimeLineEdit);
    
    endTimeLineEdit = new QLineEdit();
    formLayout->addRow("结束时间:", endTimeLineEdit);
    
    weekdaySpinBox = new QSpinBox();
    weekdaySpinBox->setRange(1, 7);
    formLayout->addRow("星期:", weekdaySpinBox);
    
    isNextSpinBox = new QSpinBox();
    isNextSpinBox->setRange(0, 1);
    formLayout->addRow("是否下一节:", isNextSpinBox);
    
    layout->addLayout(formLayout);
    
    // 按钮
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    addCourseBtn = new QPushButton("添加课程");
    updateCourseBtn = new QPushButton("更新课程");
    deleteCourseBtn = new QPushButton("删除课程");
    
    buttonLayout->addWidget(addCourseBtn);
    buttonLayout->addWidget(updateCourseBtn);
    buttonLayout->addWidget(deleteCourseBtn);
    buttonLayout->addStretch();
    
    layout->addLayout(buttonLayout);
    
    // 课程表
    courseTable = new QTableWidget();
    courseTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    courseTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    
    QStringList headers = {"ID", "教室", "课程", "教师", "时间段", "开始时间", "结束时间", "星期", "是否下一节"};
    courseTable->setColumnCount(headers.size());
    courseTable->setHorizontalHeaderLabels(headers);
    
    layout->addWidget(courseTable);
    
    // 连接信号槽
    connect(addCourseBtn, &QPushButton::clicked, this, &ServerWindow::onAddCourseClicked);
    connect(updateCourseBtn, &QPushButton::clicked, this, &ServerWindow::onUpdateCourseClicked);
    connect(deleteCourseBtn, &QPushButton::clicked, this, &ServerWindow::onDeleteCourseClicked);
    connect(courseTable, &QTableWidget::itemSelectionChanged, this, &ServerWindow::onCourseTableSelectionChanged);
    
    managementTabs->addTab(courseManagementPage, "课程管理");
    
    // 初始填充课程数据
    refreshCourseManagementData();
}

void ServerWindow::setupClassroomManagementPage() {
    classroomManagementPage = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(classroomManagementPage);
    
    // 输入表单
    QFormLayout *formLayout = new QFormLayout();
    
    roomNameLineEdit = new QLineEdit();
    formLayout->addRow("教室名称:", roomNameLineEdit);
    
    classNameLineEdit = new QLineEdit();
    formLayout->addRow("班级名称:", classNameLineEdit);
    
    capacitySpinBox = new QSpinBox();
    capacitySpinBox->setRange(1, 500);
    formLayout->addRow("容量:", capacitySpinBox);
    
    buildingLineEdit = new QLineEdit();
    formLayout->addRow("楼栋:", buildingLineEdit);
    
    floorSpinBox = new QSpinBox();
    floorSpinBox->setRange(1, 20);
    formLayout->addRow("楼层:", floorSpinBox);
    
    currentClassLineEdit = new QLineEdit();
    formLayout->addRow("当前班级:", currentClassLineEdit);
    
    layout->addLayout(formLayout);
    
    // 按钮
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    addClassroomBtn = new QPushButton("添加教室");
    updateClassroomBtn = new QPushButton("更新教室");
    deleteClassroomBtn = new QPushButton("删除教室");
    
    buttonLayout->addWidget(addClassroomBtn);
    buttonLayout->addWidget(updateClassroomBtn);
    buttonLayout->addWidget(deleteClassroomBtn);
    buttonLayout->addStretch();
    
    layout->addLayout(buttonLayout);
    
    // 教室表
    classroomTable = new QTableWidget();
    classroomTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    classroomTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    
    QStringList headers = {"ID", "教室名称", "班级名称", "容量", "楼栋", "楼层", "当前班级"};
    classroomTable->setColumnCount(headers.size());
    classroomTable->setHorizontalHeaderLabels(headers);
    
    layout->addWidget(classroomTable);
    
    // 连接信号槽
    connect(addClassroomBtn, &QPushButton::clicked, this, &ServerWindow::onAddClassroomClicked);
    connect(updateClassroomBtn, &QPushButton::clicked, this, &ServerWindow::onUpdateClassroomClicked);
    connect(deleteClassroomBtn, &QPushButton::clicked, this, &ServerWindow::onDeleteClassroomClicked);
    connect(classroomTable, &QTableWidget::itemSelectionChanged, this, &ServerWindow::onClassroomTableSelectionChanged);
    
    managementTabs->addTab(classroomManagementPage, "教室管理");
    
    // 初始填充教室数据
    refreshClassroomManagementData();
}

void ServerWindow::setupAnnouncementManagementPage() {
    announcementManagementPage = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(announcementManagementPage);
    
    // 输入表单
    QFormLayout *formLayout = new QFormLayout();
    
    titleLineEdit = new QLineEdit();
    formLayout->addRow("标题:", titleLineEdit);
    
    contentTextEdit = new QTextEdit();
    formLayout->addRow("内容:", contentTextEdit);
    
    prioritySpinBox = new QSpinBox();
    prioritySpinBox->setRange(0, 10);
    formLayout->addRow("优先级:", prioritySpinBox);
    
    publishTimeLineEdit = new QLineEdit();
    publishTimeLineEdit->setPlaceholderText("格式: yyyy-MM-dd hh:mm:ss");
    formLayout->addRow("发布时间:", publishTimeLineEdit);
    
    expireTimeLineEdit = new QLineEdit();
    expireTimeLineEdit->setPlaceholderText("格式: yyyy-MM-dd hh:mm:ss");
    formLayout->addRow("过期时间:", expireTimeLineEdit);
    
    layout->addLayout(formLayout);
    
    // 按钮
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    addAnnouncementBtn = new QPushButton("添加公告");
    updateAnnouncementBtn = new QPushButton("更新公告");
    deleteAnnouncementBtn = new QPushButton("删除公告");
    
    buttonLayout->addWidget(addAnnouncementBtn);
    buttonLayout->addWidget(updateAnnouncementBtn);
    buttonLayout->addWidget(deleteAnnouncementBtn);
    buttonLayout->addStretch();
    
    layout->addLayout(buttonLayout);
    
    // 公告表
    announcementTable = new QTableWidget();
    announcementTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    announcementTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    
    QStringList headers = {"ID", "标题", "内容", "优先级", "发布时间", "过期时间"};
    announcementTable->setColumnCount(headers.size());
    announcementTable->setHorizontalHeaderLabels(headers);
    
    layout->addWidget(announcementTable);
    
    // 连接信号槽
    connect(addAnnouncementBtn, &QPushButton::clicked, this, &ServerWindow::onAddAnnouncementClicked);
    connect(updateAnnouncementBtn, &QPushButton::clicked, this, &ServerWindow::onUpdateAnnouncementClicked);
    connect(deleteAnnouncementBtn, &QPushButton::clicked, this, &ServerWindow::onDeleteAnnouncementClicked);
    connect(announcementTable, &QTableWidget::itemSelectionChanged, this, &ServerWindow::onAnnouncementTableSelectionChanged);
    
    managementTabs->addTab(announcementManagementPage, "公告管理");
    
    // 初始填充公告数据
    refreshAnnouncementManagementData();
}

void ServerWindow::refreshCourseManagementData() {
    if (!db.isOpen()) {
        logViewer->append("数据库未打开，无法刷新课程数据");
        return;
    }
    
    QSqlQuery query(db);
    if (!query.exec("SELECT id, room, course, teacher, time_slot, start_time, end_time, weekday, is_next FROM master_schedules ORDER BY id")) {
        logViewer->append("查询课程数据失败: " + query.lastError().text());
        return;
    }
    
    // 计算行数
    QSqlQuery countQuery(db);
    countQuery.exec("SELECT COUNT(*) FROM master_schedules");
    int rowCount = 0;
    if (countQuery.next()) {
        rowCount = countQuery.value(0).toInt();
    }
    
    courseTable->setRowCount(rowCount);
    
    int row = 0;
    while (query.next()) {
        courseTable->setItem(row, 0, new QTableWidgetItem(QString::number(query.value(0).toInt())));
        courseTable->setItem(row, 1, new QTableWidgetItem(query.value(1).toString()));
        courseTable->setItem(row, 2, new QTableWidgetItem(query.value(2).toString()));
        courseTable->setItem(row, 3, new QTableWidgetItem(query.value(3).toString()));
        courseTable->setItem(row, 4, new QTableWidgetItem(query.value(4).toString()));
        courseTable->setItem(row, 5, new QTableWidgetItem(query.value(5).toString()));
        courseTable->setItem(row, 6, new QTableWidgetItem(query.value(6).toString()));
        courseTable->setItem(row, 7, new QTableWidgetItem(QString::number(query.value(7).toInt())));
        courseTable->setItem(row, 8, new QTableWidgetItem(QString::number(query.value(8).toInt())));
        row++;
    }
    
    courseTable->resizeColumnsToContents();
}

void ServerWindow::refreshClassroomManagementData() {
    if (!db.isOpen()) {
        logViewer->append("数据库未打开，无法刷新教室数据");
        return;
    }
    
    QSqlQuery query(db);
    if (!query.exec("SELECT id, room_name, class_name, capacity, building, floor, current_class FROM classrooms ORDER BY id")) {
        logViewer->append("查询教室数据失败: " + query.lastError().text());
        return;
    }
    
    // 计算行数
    QSqlQuery countQuery(db);
    countQuery.exec("SELECT COUNT(*) FROM classrooms");
    int rowCount = 0;
    if (countQuery.next()) {
        rowCount = countQuery.value(0).toInt();
    }
    
    classroomTable->setRowCount(rowCount);
    
    int row = 0;
    while (query.next()) {
        classroomTable->setItem(row, 0, new QTableWidgetItem(QString::number(query.value(0).toInt())));
        classroomTable->setItem(row, 1, new QTableWidgetItem(query.value(1).toString()));
        classroomTable->setItem(row, 2, new QTableWidgetItem(query.value(2).toString()));
        classroomTable->setItem(row, 3, new QTableWidgetItem(QString::number(query.value(3).toInt())));
        classroomTable->setItem(row, 4, new QTableWidgetItem(query.value(4).toString()));
        classroomTable->setItem(row, 5, new QTableWidgetItem(QString::number(query.value(5).toInt())));
        classroomTable->setItem(row, 6, new QTableWidgetItem(query.value(6).toString()));
        row++;
    }
    
    classroomTable->resizeColumnsToContents();
}

void ServerWindow::refreshAnnouncementManagementData() {
    if (!db.isOpen()) {
        logViewer->append("数据库未打开，无法刷新公告数据");
        return;
    }
    
    QSqlQuery query(db);
    if (!query.exec("SELECT id, title, content, priority, publish_time, expire_time FROM announcements ORDER BY id")) {
        logViewer->append("查询公告数据失败: " + query.lastError().text());
        return;
    }
    
    // 计算行数
    QSqlQuery countQuery(db);
    countQuery.exec("SELECT COUNT(*) FROM announcements");
    int rowCount = 0;
    if (countQuery.next()) {
        rowCount = countQuery.value(0).toInt();
    }
    
    announcementTable->setRowCount(rowCount);
    
    int row = 0;
    while (query.next()) {
        announcementTable->setItem(row, 0, new QTableWidgetItem(QString::number(query.value(0).toInt())));
        announcementTable->setItem(row, 1, new QTableWidgetItem(query.value(1).toString()));
        announcementTable->setItem(row, 2, new QTableWidgetItem(query.value(2).toString()));
        announcementTable->setItem(row, 3, new QTableWidgetItem(QString::number(query.value(3).toInt())));
        announcementTable->setItem(row, 4, new QTableWidgetItem(query.value(4).toString()));
        announcementTable->setItem(row, 5, new QTableWidgetItem(query.value(5).toString()));
        row++;
    }
    
    announcementTable->resizeColumnsToContents();
}

// 课程管理事件处理函数
void ServerWindow::onAddCourseClicked() {
    QString room = roomLineEdit->text().trimmed();
    QString course = courseLineEdit->text().trimmed();
    QString teacher = teacherLineEdit->text().trimmed();
    QString timeSlot = timeSlotLineEdit->text().trimmed();
    QString startTime = startTimeLineEdit->text().trimmed();
    QString endTime = endTimeLineEdit->text().trimmed();
    int weekday = weekdaySpinBox->value();
    int isNext = isNextSpinBox->value();
    
    if (room.isEmpty() || course.isEmpty() || teacher.isEmpty()) {
        logViewer->append("教室、课程和教师不能为空!");
        return;
    }
    
    if (addCourse(room, course, teacher, timeSlot, startTime, endTime, weekday, isNext)) {
        // 清空输入框
        roomLineEdit->clear();
        courseLineEdit->clear();
        teacherLineEdit->clear();
        timeSlotLineEdit->clear();
        startTimeLineEdit->clear();
        endTimeLineEdit->clear();
        weekdaySpinBox->setValue(1);
        isNextSpinBox->setValue(0);
        
        // 刷新数据
        refreshCourseManagementData();
    }
}

void ServerWindow::onUpdateCourseClicked() {
    QList<QTableWidgetItem *> selectedItems = courseTable->selectedItems();
    if (selectedItems.isEmpty()) {
        logViewer->append("请先选择要更新的课程!");
        return;
    }
    
    int row = selectedItems[0]->row();
    QTableWidgetItem *item = courseTable->item(row, 0);
    if (!item) {
        logViewer->append("无法获取要更新的课程ID!");
        return;
    }
    int id = item->text().toInt();
    
    QString room = roomLineEdit->text().trimmed();
    QString course = courseLineEdit->text().trimmed();
    QString teacher = teacherLineEdit->text().trimmed();
    QString timeSlot = timeSlotLineEdit->text().trimmed();
    QString startTime = startTimeLineEdit->text().trimmed();
    QString endTime = endTimeLineEdit->text().trimmed();
    int weekday = weekdaySpinBox->value();
    int isNext = isNextSpinBox->value();
    
    if (room.isEmpty() || course.isEmpty() || teacher.isEmpty()) {
        logViewer->append("教室、课程和教师不能为空!");
        return;
    }
    
    if (updateCourse(id, room, course, teacher, timeSlot, startTime, endTime, weekday, isNext)) {
        refreshCourseManagementData();
    }
}

void ServerWindow::onDeleteCourseClicked() {
    QList<QTableWidgetItem *> selectedItems = courseTable->selectedItems();
    if (selectedItems.isEmpty()) {
        logViewer->append("请先选择要删除的课程!");
        return;
    }
    
    int row = selectedItems[0]->row();
    QTableWidgetItem *item = courseTable->item(row, 0);
    if (!item) {
        logViewer->append("无法获取要删除的课程ID!");
        return;
    }
    int id = item->text().toInt();
    
    if (deleteCourse(id)) {
        refreshCourseManagementData();
    }
}

void ServerWindow::onCourseTableSelectionChanged() {
    QList<QTableWidgetItem *> selectedItems = courseTable->selectedItems();
    if (selectedItems.isEmpty()) {
        return;
    }
    
    int row = selectedItems[0]->row();
    
    // 将选中行的数据填入输入框
    QTableWidgetItem *item1 = courseTable->item(row, 1);
    QTableWidgetItem *item2 = courseTable->item(row, 2);
    QTableWidgetItem *item3 = courseTable->item(row, 3);
    QTableWidgetItem *item4 = courseTable->item(row, 4);
    QTableWidgetItem *item5 = courseTable->item(row, 5);
    QTableWidgetItem *item6 = courseTable->item(row, 6);
    QTableWidgetItem *item7 = courseTable->item(row, 7);
    QTableWidgetItem *item8 = courseTable->item(row, 8);
    
    if(item1) roomLineEdit->setText(item1->text());
    if(item2) courseLineEdit->setText(item2->text());
    if(item3) teacherLineEdit->setText(item3->text());
    if(item4) timeSlotLineEdit->setText(item4->text());
    if(item5) startTimeLineEdit->setText(item5->text());
    if(item6) endTimeLineEdit->setText(item6->text());
    if(item7) weekdaySpinBox->setValue(item7->text().toInt());
    if(item8) isNextSpinBox->setValue(item8->text().toInt());
}

// 教室管理事件处理函数
void ServerWindow::onAddClassroomClicked() {
    QString roomName = roomNameLineEdit->text().trimmed();
    QString className = classNameLineEdit->text().trimmed();
    int capacity = capacitySpinBox->value();
    QString building = buildingLineEdit->text().trimmed();
    int floor = floorSpinBox->value();
    QString currentClass = currentClassLineEdit->text().trimmed();
    
    if (roomName.isEmpty() || className.isEmpty()) {
        logViewer->append("教室名称和班级名称不能为空!");
        return;
    }
    
    if (addClassroom(roomName, className, capacity, building, floor, currentClass)) {
        // 清空输入框
        roomNameLineEdit->clear();
        classNameLineEdit->clear();
        capacitySpinBox->setValue(30);
        buildingLineEdit->clear();
        floorSpinBox->setValue(1);
        currentClassLineEdit->clear();
        
        // 刷新数据
        refreshClassroomManagementData();
    }
}

void ServerWindow::onUpdateClassroomClicked() {
    QList<QTableWidgetItem *> selectedItems = classroomTable->selectedItems();
    if (selectedItems.isEmpty()) {
        logViewer->append("请先选择要更新的教室!");
        return;
    }
    
    int row = selectedItems[0]->row();
    QTableWidgetItem *item = classroomTable->item(row, 1);
    if (!item) {
        logViewer->append("无法获取要更新的教室名称!");
        return;
    }
    QString roomName = item->text(); // 使用room_name作为标识
    
    QString className = classNameLineEdit->text().trimmed();
    int capacity = capacitySpinBox->value();
    QString building = buildingLineEdit->text().trimmed();
    int floor = floorSpinBox->value();
    QString currentClass = currentClassLineEdit->text().trimmed();
    
    if (roomName.isEmpty() || className.isEmpty()) {
        logViewer->append("教室名称和班级名称不能为空!");
        return;
    }
    
    if (updateClassroom(roomName, className, capacity, building, floor, currentClass)) {
        refreshClassroomManagementData();
    }
}

void ServerWindow::onDeleteClassroomClicked() {
    QList<QTableWidgetItem *> selectedItems = classroomTable->selectedItems();
    if (selectedItems.isEmpty()) {
        logViewer->append("请先选择要删除的教室!");
        return;
    }
    
    int row = selectedItems[0]->row();
    QTableWidgetItem *item = classroomTable->item(row, 1);
    if (!item) {
        logViewer->append("无法获取要删除的教室名称!");
        return;
    }
    QString roomName = item->text(); // 使用room_name作为标识
    
    if (deleteClassroom(roomName)) {
        refreshClassroomManagementData();
    }
}

void ServerWindow::onClassroomTableSelectionChanged() {
    QList<QTableWidgetItem *> selectedItems = classroomTable->selectedItems();
    if (selectedItems.isEmpty()) {
        return;
    }
    
    int row = selectedItems[0]->row();
    
    // 将选中行的数据填入输入框
    QTableWidgetItem *item1 = classroomTable->item(row, 1);
    QTableWidgetItem *item2 = classroomTable->item(row, 2);
    QTableWidgetItem *item3 = classroomTable->item(row, 3);
    QTableWidgetItem *item4 = classroomTable->item(row, 4);
    QTableWidgetItem *item5 = classroomTable->item(row, 5);
    QTableWidgetItem *item6 = classroomTable->item(row, 6);
    
    if(item1) roomNameLineEdit->setText(item1->text());
    if(item2) classNameLineEdit->setText(item2->text());
    if(item3) capacitySpinBox->setValue(item3->text().toInt());
    if(item4) buildingLineEdit->setText(item4->text());
    if(item5) floorSpinBox->setValue(item5->text().toInt());
    if(item6) currentClassLineEdit->setText(item6->text());
}

// 公告管理事件处理函数
void ServerWindow::onAddAnnouncementClicked() {
    QString title = titleLineEdit->text().trimmed();
    QString content = contentTextEdit->toPlainText().trimmed();
    int priority = prioritySpinBox->value();
    QString publishTime = publishTimeLineEdit->text().trimmed();
    QString expireTime = expireTimeLineEdit->text().trimmed();
    
    if (title.isEmpty() || content.isEmpty()) {
        logViewer->append("标题和内容不能为空!");
        return;
    }
    
    // 如果时间字段为空，使用当前时间
    if (publishTime.isEmpty()) {
        publishTime = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
        publishTimeLineEdit->setText(publishTime);
    }
    if (expireTime.isEmpty()) {
        expireTime = QDateTime::currentDateTime().addDays(7).toString("yyyy-MM-dd hh:mm:ss");
        expireTimeLineEdit->setText(expireTime);
    }
    
    if (addAnnouncement(title, content, priority, publishTime, expireTime)) {
        // 清空输入框
        titleLineEdit->clear();
        contentTextEdit->clear();
        prioritySpinBox->setValue(0);
        publishTimeLineEdit->clear();
        expireTimeLineEdit->clear();
        
        // 刷新数据
        refreshAnnouncementManagementData();
    }
}

void ServerWindow::onUpdateAnnouncementClicked() {
    QList<QTableWidgetItem *> selectedItems = announcementTable->selectedItems();
    if (selectedItems.isEmpty()) {
        logViewer->append("请先选择要更新的公告!");
        return;
    }
    
    int row = selectedItems[0]->row();
    QTableWidgetItem *item = announcementTable->item(row, 0);
    if (!item) {
        logViewer->append("无法获取要更新的公告ID!");
        return;
    }
    int id = item->text().toInt();
    
    QString title = titleLineEdit->text().trimmed();
    QString content = contentTextEdit->toPlainText().trimmed();
    int priority = prioritySpinBox->value();
    QString publishTime = publishTimeLineEdit->text().trimmed();
    QString expireTime = expireTimeLineEdit->text().trimmed();
    
    if (title.isEmpty() || content.isEmpty()) {
        logViewer->append("标题和内容不能为空!");
        return;
    }
    
    if (updateAnnouncement(id, title, content, priority, publishTime, expireTime)) {
        refreshAnnouncementManagementData();
    }
}

void ServerWindow::onDeleteAnnouncementClicked() {
    QList<QTableWidgetItem *> selectedItems = announcementTable->selectedItems();
    if (selectedItems.isEmpty()) {
        logViewer->append("请先选择要删除的公告!");
        return;
    }
    
    int row = selectedItems[0]->row();
    QTableWidgetItem *item = announcementTable->item(row, 0);
    if (!item) {
        logViewer->append("无法获取要删除的公告ID!");
        return;
    }
    int id = item->text().toInt();
    
    if (deleteAnnouncement(id)) {
        refreshAnnouncementManagementData();
    }
}

void ServerWindow::onAnnouncementTableSelectionChanged() {
    QList<QTableWidgetItem *> selectedItems = announcementTable->selectedItems();
    if (selectedItems.isEmpty()) {
        return;
    }
    
    int row = selectedItems[0]->row();
    
    // 将选中行的数据填入输入框
    QTableWidgetItem *item1 = announcementTable->item(row, 1);
    QTableWidgetItem *item2 = announcementTable->item(row, 2);
    QTableWidgetItem *item3 = announcementTable->item(row, 3);
    QTableWidgetItem *item4 = announcementTable->item(row, 4);
    QTableWidgetItem *item5 = announcementTable->item(row, 5);
    
    if(item1) titleLineEdit->setText(item1->text());
    if(item2) contentTextEdit->setText(item2->text());
    if(item3) prioritySpinBox->setValue(item3->text().toInt());
    if(item4) publishTimeLineEdit->setText(item4->text());
    if(item5) expireTimeLineEdit->setText(item5->text());
}
