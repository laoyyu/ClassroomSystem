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
    
    QTabWidget *dataTabWidget;    // 数据显示标签页
    QTableWidget *schedulesTable;  // 课程表显示
    QTableWidget *classroomsTable; // 教室表显示
    QTableWidget *announcementsTable; // 公告表显示
    QPushButton *refreshButton;    // 刷新按钮
    QPushButton *clearFilterButton; // 清除筛选按钮
    QLabel *statusLabel;           // 状态标签
    QComboBox *weekDayFilterCombo;  // 星期筛选下拉框

    QTcpServer *tcpServer;
    QTextEdit *logViewer;
    QSqlDatabase db;
    QTimer *classUpdateTimer;          // 班级信息更新定时器
    
    // 用于跟踪客户端连接
    QSet<QTcpSocket*> clientSockets;
};

#endif // SERVERWINDOW_H