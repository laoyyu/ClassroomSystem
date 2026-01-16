#ifndef SERVERWINDOW_H
#define SERVERWINDOW_H

#include <QSqlDatabase>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTextEdit>
#include <QWidget>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QTableWidget>
#include <QLabel>
#include <QComboBox>
#include <QTime>
#include <QDate>
#include <QTimer>
#include <QSet>
#include <QLineEdit>
#include <QSpinBox>
#include <QString>
#include <QMap>
#include <QVector>

class ServerWindow : public QWidget
{
    Q_OBJECT

public:
    ServerWindow(QWidget *parent = nullptr);
    ~ServerWindow();

private slots:
    void onNewConnection();
    void onReadClientData();
    void onClientDisconnected();

private:
    void initDb();                // 初始化服务端数据库
    void initSampleData();        // 初始化示例数据
    QByteArray getScheduleJson(); // 从数据库获取数据并转为JSON
    void setupUi();               // 设置用户界面
    void refreshData();           // 刷新数据显示
    void populateSchedulesTable(); // 填充课程表数据
    void populateClassroomsTable(); // 填充教室表数据
    void populateAnnouncementsTable(); // 填充公告表数据
    void populateWeekDayFilter();      // 填充星期筛选下拉框
    void filterSchedulesByWeekday();   // 按星期筛选课程表
    void onWeekDayFilterChanged();     // 星期筛选变化槽函数
    void updateCurrentClasses();       // 更新当前上课班级信息
    
    // 管理界面相关函数
    void setupManagementUi();
    void setupCourseManagementPage();
    void setupClassroomManagementPage();
    void setupAnnouncementManagementPage();
    
    void refreshCourseManagementData();
    void refreshClassroomManagementData();
    void refreshAnnouncementManagementData();
    
    // 课程管理事件处理函数
    void onAddCourseClicked();
    void onUpdateCourseClicked();
    void onDeleteCourseClicked();
    void onCourseTableSelectionChanged();
    
    // 教室管理事件处理函数
    void onAddClassroomClicked();
    void onUpdateClassroomClicked();
    void onDeleteClassroomClicked();
    void onClassroomTableSelectionChanged();
    
    // 公告管理事件处理函数
    void onAddAnnouncementClicked();
    void onUpdateAnnouncementClicked();
    void onDeleteAnnouncementClicked();
    void onAnnouncementTableSelectionChanged();
    
    // CRUD 操作相关函数
    bool addCourse(const QString& room, const QString& course, const QString& teacher, 
                  const QString& timeSlot, const QString& startTime, const QString& endTime, 
                  int weekday, int isNext = 0);
    bool updateCourse(int id, const QString& room, const QString& course, const QString& teacher, 
                      const QString& timeSlot, const QString& startTime, const QString& endTime, 
                      int weekday, int isNext);
    bool deleteCourse(int id);
    
    bool addClassroom(const QString& roomName, const QString& className, int capacity, 
                      const QString& building, int floor, const QString& currentClass = "");
    bool updateClassroom(const QString& roomName, const QString& className, int capacity, 
                         const QString& building, int floor, const QString& currentClass);
    bool deleteClassroom(const QString& roomName);
    
    bool addAnnouncement(const QString& title, const QString& content, int priority, 
                         const QString& publishTime, const QString& expireTime);
    bool updateAnnouncement(int id, const QString& title, const QString& content, int priority, 
                           const QString& publishTime, const QString& expireTime);
    bool deleteAnnouncement(int id);
    
    QTabWidget *dataTabWidget;    // 数据显示标签页
    QTableWidget *schedulesTable;  // 课程表显示
    QTableWidget *classroomsTable; // 教室表显示
    QTableWidget *announcementsTable; // 公告表显示
    QPushButton *refreshButton;    // 刷新按钮
    QPushButton *clearFilterButton; // 清除筛选按钮
    QLabel *statusLabel;           // 状态标签
    QComboBox *weekDayFilterCombo;  // 星期筛选下拉框
    
    // 管理界面组件
    QWidget *managementWidget;       // 管理界面主窗口
    QTabWidget *managementTabs;      // 管理功能标签页
    QWidget *courseManagementPage;   // 课程管理页面
    QWidget *classroomManagementPage; // 教室管理页面
    QWidget *announcementManagementPage; // 公告管理页面
    
    // 课程管理界面元素
    QLineEdit *roomLineEdit;
    QLineEdit *courseLineEdit;
    QLineEdit *teacherLineEdit;
    QLineEdit *timeSlotLineEdit;
    QLineEdit *startTimeLineEdit;
    QLineEdit *endTimeLineEdit;
    QSpinBox *weekdaySpinBox;
    QSpinBox *isNextSpinBox;
    QTableWidget *courseTable;
    QPushButton *addCourseBtn;
    QPushButton *updateCourseBtn;
    QPushButton *deleteCourseBtn;
    
    // 教室管理界面元素
    QLineEdit *roomNameLineEdit;
    QLineEdit *classNameLineEdit;
    QSpinBox *capacitySpinBox;
    QLineEdit *buildingLineEdit;
    QSpinBox *floorSpinBox;
    QLineEdit *currentClassLineEdit;
    QTableWidget *classroomTable;
    QPushButton *addClassroomBtn;
    QPushButton *updateClassroomBtn;
    QPushButton *deleteClassroomBtn;
    
    // 公告管理界面元素
    QLineEdit *titleLineEdit;
    QTextEdit *contentTextEdit;
    QSpinBox *prioritySpinBox;
    QLineEdit *publishTimeLineEdit;
    QLineEdit *expireTimeLineEdit;
    QTableWidget *announcementTable;
    QPushButton *addAnnouncementBtn;
    QPushButton *updateAnnouncementBtn;
    QPushButton *deleteAnnouncementBtn;

    QTcpServer *tcpServer;
    QTextEdit *logViewer;
    QSqlDatabase db;
    QTimer *classUpdateTimer;          // 班级信息更新定时器
    
    // 用于跟踪客户端连接
    QSet<QTcpSocket*> clientSockets;
};

#endif // SERVERWINDOW_H