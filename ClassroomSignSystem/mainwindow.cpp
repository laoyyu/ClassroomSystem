#include "MainWindow.h"
#include "DatabaseManager.h"
#include "NetworkWorker.h"
#include <QDateTime>
#include <QTimer>
#include <QSqlQuery>
#include <QTabWidget>
#include <QSplitter>

MainWindow::MainWindow(QWidget *parent)
    : QWidget(parent), scrollPosition(0)
{
    DatabaseManager::initDb();

    setupUi();
    setupModel();
    startWorker();

    timeTimer = new QTimer(this);
    connect(timeTimer, &QTimer::timeout, this, &MainWindow::updateCurrentTime);
    timeTimer->start(1000);

    scrollTimer = new QTimer(this);
    connect(scrollTimer, &QTimer::timeout, this, &MainWindow::scrollAnnouncement);
    scrollTimer->start(200);

    updateCurrentTime();
}

MainWindow::~MainWindow()
{
    if (workerThread) {
        workerThread->quit();
        workerThread->wait();
    }
}

void MainWindow::setupUi() {
    QHBoxLayout *mainLayout = new QHBoxLayout(this);

    QGroupBox *infoGroup = new QGroupBox("智慧教室班牌 - Class 101");
    QVBoxLayout *infoLayout = new QVBoxLayout;

    lblCurrentTime = new QLabel();
    lblCurrentTime->setStyleSheet("font-size: 16px; color: #3498db; font-weight: bold;");
    lblCurrentTime->setAlignment(Qt::AlignCenter);

    lblCourseName = new QLabel("Loading...");
    lblCourseName->setStyleSheet("font-size: 36px; font-weight: bold; color: #2c3e50; padding: 20px;");
    lblCourseName->setAlignment(Qt::AlignCenter);

    lblTeacher = new QLabel("教师: --");
    lblTeacher->setStyleSheet("font-size: 22px; color: #e74c3c; padding: 10px;");
    lblTeacher->setAlignment(Qt::AlignCenter);

    lblTime = new QLabel("时间: --");
    lblTime->setStyleSheet("font-size: 18px; color: #7f8c8d; padding: 5px;");
    lblTime->setAlignment(Qt::AlignCenter);

    QFrame *line = new QFrame();
    line->setFrameShape(QFrame::HLine);
    line->setStyleSheet("background-color: #bdc3c7;");

    lblNextCourse = new QLabel("下节预告: --");
    lblNextCourse->setStyleSheet("font-size: 16px; color: #16a085; font-style: italic; padding: 10px;");
    lblNextCourse->setAlignment(Qt::AlignCenter);

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

    QTableView *classroomView = new QTableView();
    classroomView->setAlternatingRowColors(true);
    classroomView->setSelectionBehavior(QAbstractItemView::SelectRows);
    classroomView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    classroomView->verticalHeader()->setVisible(false);

    classroomLayout->addWidget(classroomView);

    tabWidget->addTab(scheduleTab, "课程表");
    tabWidget->addTab(classroomTab, "教室信息");

    QVBoxLayout *rightLayout = new QVBoxLayout();
    rightLayout->addWidget(lblAnnouncement);
    rightLayout->addWidget(tabWidget);

    mainLayout->addWidget(infoGroup, 1);
    mainLayout->addLayout(rightLayout, 2);

    resize(1400, 800);
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

    QTableView *classroomView = findChild<QTableView*>();
    if (classroomView && classroomView != tableView) {
        classroomView->setModel(classroomModel);
        classroomView->hideColumn(0);
    }
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

    model->select();
    classroomModel->select();

    updateDisplay();
}

void MainWindow::onAnnouncementUpdated(const QString &title, const QString &content) {
    announcementText = "【" + title + "】" + content;
    scrollPosition = 0;
    lblAnnouncement->setText(announcementText);
}

void MainWindow::updateDisplay() {
    QSqlQuery query;

    query.prepare("SELECT course_name, teacher, time_slot FROM schedules WHERE room_name = 'Class 101' AND is_next = 0 LIMIT 1");
    if(query.exec() && query.next()) {
        lblCourseName->setText(query.value(0).toString());
        lblTeacher->setText("教师: " + query.value(1).toString());
        lblTime->setText("时间: " + query.value(2).toString());
    } else {
        lblCourseName->setText("当前无课");
        lblTeacher->setText("");
        lblTime->setText("");
    }

    query.prepare("SELECT course_name, time_slot FROM schedules WHERE room_name = 'Class 101' AND is_next = 1 LIMIT 1");
    if(query.exec() && query.next()) {
        lblNextCourse->setText("下节预告: " + query.value(0).toString() + " (" + query.value(1).toString() + ")");
    } else {
        lblNextCourse->setText("下节预告: 无");
    }
}

void MainWindow::filterData(const QString &text) {
    QString filterStr = QString("room_name LIKE '%%1%' OR teacher LIKE '%%1%' OR course_name LIKE '%%1%'").arg(text);
    model->setFilter(filterStr);
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
