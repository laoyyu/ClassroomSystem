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
    titleLayout->addWidget(new QLabel("当前班级:"));
    classroomComboBox = new QComboBox();
    classroomComboBox->setStyleSheet("font-size: 14px; padding: 5px; min-width: 150px;");
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
    tableView->hideColumn(0);
    tableView->hideColumn(5);

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

    query.prepare("SELECT course_name, teacher, time_slot FROM schedules WHERE room_name = ? AND is_next = 0 LIMIT 1");
    query.addBindValue(currentRoomName);
    if(query.exec() && query.next()) {
        lblCourseName->setText(query.value(0).toString());
        lblTeacher->setText("教师: " + query.value(1).toString());
        lblTime->setText("时间: " + query.value(2).toString());
    } else {
        lblCourseName->setText("当前无课");
        lblTeacher->setText("");
        lblTime->setText("");
    }

    query.prepare("SELECT course_name, time_slot FROM schedules WHERE room_name = ? AND is_next = 1 LIMIT 1");
    query.addBindValue(currentRoomName);
    if(query.exec() && query.next()) {
        lblNextCourse->setText("下节预告: " + query.value(0).toString() + " (" + query.value(1).toString() + ")");
    } else {
        lblNextCourse->setText("下节预告: 无");
    }
}

void MainWindow::filterData(const QString &text) {
    if (text.isEmpty()) {
        // 搜索框为空，显示所有数据
        model->setFilter("");
    } else {
        // 正确的 SQL LIKE 语法：使用 % 作为通配符
        QString filterStr = QString("room_name LIKE '%%1%' OR teacher LIKE '%%1%' OR course_name LIKE '%%1%'").arg(text);
        model->setFilter(filterStr);
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
    QSqlQuery query;
    query.prepare("SELECT room_name, class_name FROM classrooms ORDER BY room_name");
    if (query.exec()) {
        classroomComboBox->clear();
        while (query.next()) {
            QString roomName = query.value(0).toString();
            QString className = query.value(1).toString();
            classroomComboBox->addItem(roomName + " - " + className, roomName);
        }
        if (classroomComboBox->count() > 0) {
            classroomComboBox->setCurrentIndex(0);
        }
    }
}

void MainWindow::onClassroomChanged(int index) {
    if (index >= 0) {
        QString roomName = classroomComboBox->currentData().toString();
        updateDisplay(roomName);
    }
}
