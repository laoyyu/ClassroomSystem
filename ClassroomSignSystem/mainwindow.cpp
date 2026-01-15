#include "mainwindow.h"
#include "DatabaseManager.h"
#include "networkworker.h"
#include <QDateTime>
#include <QTimer>
#include <QSqlQuery>
#include <QTabWidget>
#include <QSplitter>
#include <QComboBox>

MainWindow::MainWindow(QWidget *parent)
    : QWidget(parent), scrollPosition(0)
{
    qDebug() << "MainWindow构造函数开始";

    if (!DatabaseManager::initDb()) {
        qDebug() << "数据库初始化失败";
    } else {
        qDebug() << "数据库初始化成功";
    }

    qDebug() << "开始设置UI";
    setupUi();

    qDebug() << "开始设置Model";
    setupModel();

    qDebug() << "开始启动Worker";
    startWorker();

    timeTimer = new QTimer(this);
    connect(timeTimer, &QTimer::timeout, this, &MainWindow::updateCurrentTime);
    timeTimer->start(1000);

    scrollTimer = new QTimer(this);
    connect(scrollTimer, &QTimer::timeout, this, &MainWindow::scrollAnnouncement);
    scrollTimer->start(200);

    bottomScrollTimer = new QTimer(this);
    connect(bottomScrollTimer, &QTimer::timeout, this, &MainWindow::scrollBottomNotification);
    bottomScrollTimer->start(100);

    qDebug() << "开始更新显示";
    updateCurrentTime();
    loadClassrooms();
    updateDisplay();
    loadAnnouncement();
    loadNotifications();

    qDebug() << "MainWindow构造函数完成";
}

MainWindow::~MainWindow()
{
    if (workerThread) {
        workerThread->quit();
        workerThread->wait();
    }
}

void MainWindow::setupUi() {
    QGroupBox *infoGroup = new QGroupBox("智慧教室班牌");
    QVBoxLayout *infoLayout = new QVBoxLayout;

    QHBoxLayout *titleLayout = new QHBoxLayout;
    
    // 优化标签样式
    QLabel *lblClassroom = new QLabel("当前教室:");
    lblClassroom->setStyleSheet("font-size: 15px; font-weight: bold; color: #34495e;");
    titleLayout->addWidget(lblClassroom);
    
    // 优化下拉框样式
    classroomComboBox = new QComboBox();
    classroomComboBox->setStyleSheet(
        "QComboBox {"
        "    font-size: 15px;"
        "    padding: 8px 12px;"
        "    min-width: 280px;"
        "    border: 2px solid #3498db;"
        "    border-radius: 8px;"
        "    background-color: white;"
        "    color: #2c3e50;"
        "    font-weight: 500;"
        "}"
        "QComboBox:hover {"
        "    border-color: #2980b9;"
        "    background-color: #ecf0f1;"
        "}"
        "QComboBox:focus {"
        "    border-color: #2980b9;"
        "    background-color: #e8f4f8;"
        "}"
        "QComboBox::drop-down {"
        "    border: none;"
        "    width: 30px;"
        "}"
        "QComboBox::down-arrow {"
        "    image: none;"
        "    border-left: 5px solid transparent;"
        "    border-right: 5px solid transparent;"
        "    border-top: 6px solid #3498db;"
        "    margin-right: 10px;"
        "}"
        "QComboBox QAbstractItemView {"
        "    border: 2px solid #3498db;"
        "    border-radius: 5px;"
        "    background-color: white;"
        "    selection-background-color: #3498db;"
        "    selection-color: white;"
        "    font-size: 14px;"
        "    padding: 5px;"
        "    outline: none;"
        "}"
        "QComboBox QAbstractItemView::item {"
        "    min-height: 35px;"
        "    padding: 8px 12px;"
        "    border-bottom: 1px solid #ecf0f1;"
        "}"
        "QComboBox QAbstractItemView::item:hover {"
        "    background-color: #e8f4f8;"
        "    color: #2c3e50;"
        "}"
        "QComboBox QAbstractItemView::item:selected {"
        "    background-color: #3498db;"
        "    color: white;"
        "}"
    );
    classroomComboBox->setMaxVisibleItems(10);
    titleLayout->addWidget(classroomComboBox);
    titleLayout->addStretch();

    connect(classroomComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::onClassroomChanged);

    lblCurrentTime = new QLabel();
    lblCurrentTime->setStyleSheet("font-size: 14px; color: #2c3e50; font-weight: bold;");
    lblCurrentTime->setAlignment(Qt::AlignCenter);

    lblCourseName = new QLabel("Loading...");
    lblCourseName->setStyleSheet("font-size: 28px; font-weight: bold; color: #2c3e50; padding: 15px;");
    lblCourseName->setAlignment(Qt::AlignCenter);

    lblTeacher = new QLabel("教师: --");
    lblTeacher->setStyleSheet("font-size: 18px; color: #e74c3c; padding: 8px;");
    lblTeacher->setAlignment(Qt::AlignCenter);

    lblTime = new QLabel("时间: --");
    lblTime->setStyleSheet("font-size: 16px; color: #7f8c8d; padding: 5px;");
    lblTime->setAlignment(Qt::AlignCenter);

    QFrame *line = new QFrame();
    line->setFrameShape(QFrame::HLine);
    line->setStyleSheet("background-color: #bdc3c7;");

    lblNextCourse = new QLabel("下节预告: --");
    lblNextCourse->setStyleSheet("font-size: 14px; color: #16a085; font-style: italic; padding: 8px;");
    lblNextCourse->setAlignment(Qt::AlignCenter);

    infoLayout->addLayout(titleLayout);
    infoLayout->addWidget(lblCurrentTime);
    infoLayout->addWidget(lblCourseName);
    infoLayout->addWidget(lblTeacher);
    infoLayout->addWidget(lblTime);
    infoLayout->addWidget(line);
    infoLayout->addWidget(lblNextCourse);
    infoLayout->addStretch();
    infoGroup->setLayout(infoLayout);

    lblAnnouncement = new QLabel();
    lblAnnouncement->setStyleSheet("background-color: #f39c12; color: white; padding: 10px; font-size: 14px; border-radius: 5px;");
    lblAnnouncement->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    lblAnnouncement->setMinimumHeight(40);
    lblAnnouncement->setWordWrap(false);
    lblAnnouncement->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    QTabWidget *tabWidget = new QTabWidget();

    QWidget *scheduleTab = new QWidget();
    QVBoxLayout *scheduleLayout = new QVBoxLayout(scheduleTab);

    QHBoxLayout *filterLayout = new QHBoxLayout;
    filterLayout->addWidget(new QLabel("搜索教室/教师:"));
    searchBox = new QLineEdit();
    searchBox->setPlaceholderText("输入关键词筛选...");
    filterLayout->addWidget(searchBox);

    connect(searchBox, &QLineEdit::textChanged, this, &MainWindow::filterData);

    tableView = new QTableView();
    tableView->setAlternatingRowColors(true);
    tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    tableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    tableView->verticalHeader()->setVisible(false);

    lblStatus = new QLabel("等待同步...");
    lblStatus->setStyleSheet("color: #7f8c8d; font-style: italic;");

    scheduleLayout->addLayout(filterLayout);
    scheduleLayout->addWidget(tableView);
    scheduleLayout->addWidget(lblStatus);

    QWidget *classroomTab = new QWidget();
    QVBoxLayout *classroomLayout = new QVBoxLayout(classroomTab);

    classroomView = new QTableView();
    classroomView->setAlternatingRowColors(true);
    classroomView->setSelectionBehavior(QAbstractItemView::SelectRows);
    classroomView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    classroomView->verticalHeader()->setVisible(false);

    classroomLayout->addWidget(classroomView);

    tabWidget->addTab(scheduleTab, "课程表");
    tabWidget->addTab(classroomTab, "教室信息");
    tabWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    QVBoxLayout *rightLayout = new QVBoxLayout();
    rightLayout->addWidget(lblAnnouncement);
    rightLayout->addWidget(tabWidget);

    QVBoxLayout *overallLayout = new QVBoxLayout(this);

    QHBoxLayout *contentLayout = new QHBoxLayout();
    contentLayout->addWidget(infoGroup, 1);
    contentLayout->addLayout(rightLayout, 2);

    overallLayout->addLayout(contentLayout);

    lblBottomNotification = new QLabel();
    lblBottomNotification->setStyleSheet("background-color: #2c3e50; color: #ecf0f1; padding: 15px; font-size: 16px; font-weight: bold;");
    lblBottomNotification->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    lblBottomNotification->setMinimumHeight(50);
    lblBottomNotification->setWordWrap(false);
    lblBottomNotification->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    overallLayout->addWidget(lblBottomNotification);

    resize(1200, 700);
}

void MainWindow::setupModel() {
    model = new QSqlTableModel(this);
    model->setTable("schedules");
    model->setHeaderData(1, Qt::Horizontal, "教室");
    model->setHeaderData(2, Qt::Horizontal, "课程");
    model->setHeaderData(3, Qt::Horizontal, "教师");
    model->setHeaderData(4, Qt::Horizontal, "时间段");
    model->select();

    tableView->setModel(model);
    tableView->hideColumn(0);  // id
    tableView->hideColumn(5);  // start_time
    tableView->hideColumn(6);  // end_time
    tableView->hideColumn(7);  // weekday
    tableView->hideColumn(8);  // is_next

    classroomModel = new QSqlTableModel(this);
    classroomModel->setTable("classrooms");
    classroomModel->setHeaderData(1, Qt::Horizontal, "教室名称");
    classroomModel->setHeaderData(2, Qt::Horizontal, "班级");
    classroomModel->setHeaderData(3, Qt::Horizontal, "容量");
    classroomModel->setHeaderData(4, Qt::Horizontal, "教学楼");
    classroomModel->setHeaderData(5, Qt::Horizontal, "楼层");
    classroomModel->select();

    classroomView->setModel(classroomModel);
    classroomView->hideColumn(0);
}

void MainWindow::startWorker() {
    workerThread = new QThread;
    NetworkWorker *worker = new NetworkWorker;

    worker->moveToThread(workerThread);

    connect(workerThread, &QThread::started, worker, &NetworkWorker::startSync);
    connect(worker, &NetworkWorker::dataUpdated, this, &MainWindow::onDataSynced);
    connect(worker, &NetworkWorker::announcementUpdated, this, &MainWindow::onAnnouncementUpdated);

    connect(workerThread, &QThread::finished, worker, &QObject::deleteLater);
    connect(workerThread, &QThread::finished, workerThread, &QObject::deleteLater);

    workerThread->start();
}

void MainWindow::onDataSynced(const QString &msg) {
    lblStatus->setText(msg);

    // 清除过滤条件，避免之前的过滤影响新数据
    model->setFilter("");
    model->select();
    
    classroomModel->setFilter("");
    classroomModel->select();

    loadClassrooms();
    updateDisplay();
    
    // 同步后自动过滤到当前选中的教室
    if (classroomComboBox->count() > 0) {
        QString currentRoom = classroomComboBox->currentData().toString();
        if (!currentRoom.isEmpty()) {
            QString filterStr = QString("room_name = '%1'").arg(currentRoom);
            model->setFilter(filterStr);
            model->select();
        }
    }
}

void MainWindow::onAnnouncementUpdated(const QString &title, const QString &content) {
    announcementText = "【" + title + "】" + content;
    scrollPosition = 0;
    lblAnnouncement->setText(announcementText);
}

void MainWindow::updateDisplay(const QString &roomName) {
    QString currentRoomName = roomName;
    if (currentRoomName.isEmpty() && classroomComboBox->count() > 0) {
        currentRoomName = classroomComboBox->currentData().toString();
    }
    
    if (currentRoomName.isEmpty()) {
        currentRoomName = "Class 101";
    }

    QSqlQuery query;
    QDateTime now = QDateTime::currentDateTime();
    QString currentTime = now.toString("HH:mm");
    int currentWeekday = now.date().dayOfWeek();
    
    // 查询当前时间段的课程（当前时间在 start_time 和 end_time 之间）
    query.prepare(
        "SELECT course_name, teacher, time_slot, start_time, end_time "
        "FROM schedules "
        "WHERE room_name = ? AND weekday = ? AND start_time <= ? AND end_time >= ? "
        "ORDER BY start_time LIMIT 1"
    );
    query.addBindValue(currentRoomName);
    query.addBindValue(currentWeekday);
    query.addBindValue(currentTime);
    query.addBindValue(currentTime);
    
    if(query.exec() && query.next()) {
        // 找到了当前正在进行的课程
        lblCourseName->setText(query.value(0).toString());
        lblTeacher->setText("教师: " + query.value(1).toString());
        lblTime->setText("时间: " + query.value(2).toString());
        
        QString currentEndTime = query.value(4).toString();
        
        // 查询下一节课（start_time > 当前课程的 end_time）
        QSqlQuery nextQuery;
        nextQuery.prepare(
            "SELECT course_name, time_slot, weekday "
            "FROM schedules "
            "WHERE room_name = ? AND (weekday > ? OR (weekday = ? AND start_time > ?)) "
            "ORDER BY weekday ASC, start_time ASC LIMIT 1"
        );
        nextQuery.addBindValue(currentRoomName);
        nextQuery.addBindValue(currentWeekday);
        nextQuery.addBindValue(currentWeekday);
        nextQuery.addBindValue(currentEndTime);
        
        if(nextQuery.exec() && nextQuery.next()) {
            QString nextCourseName = nextQuery.value(0).toString();
            QString nextTimeSlot = nextQuery.value(1).toString();
            int nextWeekday = nextQuery.value(2).toInt();
            
            QString dayName;
            switch(nextWeekday) {
                case 1: dayName = "周一"; break;
                case 2: dayName = "周二"; break;
                case 3: dayName = "周三"; break;
                case 4: dayName = "周四"; break;
                case 5: dayName = "周五"; break;
                case 6: dayName = "周六"; break;
                case 7: dayName = "周日"; break;
                default: dayName = "未知"; break;
            }
            
            lblNextCourse->setText("下节预告: " + nextCourseName + " (" + dayName + " " + nextTimeSlot + ")");
        } else {
            lblNextCourse->setText("下节预告: 无");
        }
    } else {
        // 当前时间没有课，查找下一节课（start_time > 当前时间）
        query.prepare(
            "SELECT course_name, teacher, time_slot, start_time, weekday "
            "FROM schedules "
            "WHERE room_name = ? AND (weekday > ? OR (weekday = ? AND start_time > ?)) "
            "ORDER BY weekday ASC, start_time ASC LIMIT 1"
        );
        query.addBindValue(currentRoomName);
        query.addBindValue(currentWeekday);
        query.addBindValue(currentWeekday);
        query.addBindValue(currentTime);
        
        if(query.exec() && query.next()) {
            // 显示下一节课
            QString nextCourseName = query.value(0).toString();
            QString nextTimeSlot = query.value(2).toString();
            int nextWeekday = query.value(4).toInt();
            
            QString dayName;
            switch(nextWeekday) {
                case 1: dayName = "周一"; break;
                case 2: dayName = "周二"; break;
                case 3: dayName = "周三"; break;
                case 4: dayName = "周四"; break;
                case 5: dayName = "周五"; break;
                case 6: dayName = "周六"; break;
                case 7: dayName = "周日"; break;
                default: dayName = "未知"; break;
            }
            
            lblCourseName->setText("当前无课");
            lblTeacher->setText("");
            lblTime->setText("");
            lblNextCourse->setText("下节预告: " + nextCourseName + " (" + dayName + " " + nextTimeSlot + ")");
        } else {
            // 今天没有更多课程了
            lblCourseName->setText("当前无课");
            lblTeacher->setText("");
            lblTime->setText("");
            lblNextCourse->setText("下节预告: 无");
        }
    }
}

void MainWindow::filterData(const QString &text) {
    // 如果搜索框有内容，优先使用搜索过滤
    if (!text.isEmpty()) {
        // 正确的 SQL LIKE 语法：使用 % 作为通配符
        QString filterStr = QString("room_name LIKE '%%1%' OR teacher LIKE '%%1%' OR course_name LIKE '%%1%'").arg(text);
        model->setFilter(filterStr);
    } else {
        // 搜索框为空，恢复到当前选中的教室过滤
        if (classroomComboBox->count() > 0) {
            QString currentRoom = classroomComboBox->currentData().toString();
            if (!currentRoom.isEmpty()) {
                QString filterStr = QString("room_name = '%1'").arg(currentRoom);
                model->setFilter(filterStr);
            } else {
                model->setFilter("");
            }
        } else {
            model->setFilter("");
        }
    }
    model->select();
}

void MainWindow::updateCurrentTime() {
    QDateTime now = QDateTime::currentDateTime();
    lblCurrentTime->setText(now.toString("yyyy年MM月dd日 HH:mm:ss dddd"));
}

void MainWindow::scrollAnnouncement() {
    if (announcementText.isEmpty()) {
        return;
    }

    QFontMetrics fm(lblAnnouncement->font());
    int textWidth = fm.horizontalAdvance(announcementText);
    int labelWidth = lblAnnouncement->width();

    if (textWidth > labelWidth) {
        scrollPosition += 2;
        if (scrollPosition > textWidth) {
            scrollPosition = -labelWidth;
        }
        lblAnnouncement->setText(announcementText.right(textWidth - scrollPosition) + "    " + announcementText);
    }
}

void MainWindow::loadAnnouncement() {
    QSqlQuery query;
    query.prepare("SELECT title, content FROM announcements ORDER BY priority DESC, publish_time DESC LIMIT 1");
    if (query.exec() && query.next()) {
        QString title = query.value(0).toString();
        QString content = query.value(1).toString();
        announcementText = "【" + title + "】" + content;
        scrollPosition = 0;
        lblAnnouncement->setText(announcementText);
    }
}

void MainWindow::scrollBottomNotification() {
    if (notificationText.isEmpty()) {
        return;
    }

    bottomScrollPosition++;
    if (bottomScrollPosition >= notificationText.length()) {
        bottomScrollPosition = 0;
    }

    QString displayText = notificationText.mid(bottomScrollPosition) + notificationText.left(bottomScrollPosition);
    lblBottomNotification->setText(displayText);
}

void MainWindow::loadNotifications() {
    notificationText = "📢 期末考试1月15日开始  ★  图书馆8:00-22:00  ★  周六凌晨网络维护  ★  寒假1月20日-2月20日  ★  选课1月10日开放  ★  请同学们注意考试时间  ★  祝大家考试顺利  ★  考试期间请保持安静  ★  提前30分钟到达考场  ★  携带好准考证和身份证  ★  ";
    bottomScrollPosition = 0;
    lblBottomNotification->setText(notificationText);
}

void MainWindow::loadClassrooms() {
    // 保存当前选中的值
    QString currentSelectedRoom = "";
    if (classroomComboBox->currentIndex() >= 0) {
        currentSelectedRoom = classroomComboBox->currentData().toString();
    }
    
    QSqlQuery query;
    query.prepare("SELECT room_name, class_name FROM classrooms ORDER BY room_name");
    if (query.exec()) {
        classroomComboBox->clear();
        while (query.next()) {
            QString roomName = query.value(0).toString();
            QString className = query.value(1).toString();
            classroomComboBox->addItem(roomName + " - " + className, roomName);
        }
        
        // 尝试恢复之前选中的值
        if (!currentSelectedRoom.isEmpty()) {
            int index = classroomComboBox->findData(currentSelectedRoom);
            if (index >= 0) {
                classroomComboBox->setCurrentIndex(index);
            } else {
                // 如果找不到之前选中的值，则选择第一个
                if (classroomComboBox->count() > 0) {
                    classroomComboBox->setCurrentIndex(0);
                }
            }
        } else {
            // 如果之前没有选择任何值，且列表不为空，则选择第一个
            if (classroomComboBox->count() > 0) {
                classroomComboBox->setCurrentIndex(0);
            }
        }
    }
}

void MainWindow::onClassroomChanged(int index) {
    if (index >= 0) {
        QString roomName = classroomComboBox->currentData().toString();
        
        // 更新左侧当前课程显示
        updateDisplay(roomName);
        
        // 同时过滤右侧课程表，只显示当前教室的课程
        if (!roomName.isEmpty()) {
            QString filterStr = QString("room_name = '%1'").arg(roomName);
            model->setFilter(filterStr);
            model->select();
        }
    }
}
