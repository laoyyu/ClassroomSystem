#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QWidget>
#include <QLabel>
#include <QTableView>
#include <QSqlTableModel>
#include <QLineEdit>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QThread>
#include <QTimer>
#include <QComboBox>

class MainWindow : public QWidget
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void updateDisplay(const QString &roomName = QString());
    void onDataSynced(const QString &msg);
    void onAnnouncementUpdated(const QString &title, const QString &content);
    void filterData(const QString &text);
    void updateCurrentTime();
    void scrollAnnouncement();
    void scrollBottomNotification();
    void loadAnnouncement();
    void loadNotifications();
    void onClassroomChanged(int index);

private:
    void setupUi();
    void setupModel();
    void startWorker();
    void loadClassrooms();

    QLabel *lblCourseName;
    QLabel *lblTeacher;
    QLabel *lblTime;
    QLabel *lblNextCourse;
    QLabel *lblStatus;
    QLabel *lblAnnouncement;
    QLabel *lblCurrentTime;
    QLabel *lblBottomNotification;

    QLineEdit *searchBox;
    QComboBox *classroomComboBox;
    QTableView *tableView;
    QTableView *classroomView;

    QSqlTableModel *model;
    QSqlTableModel *classroomModel;

    QThread *workerThread;
    QTimer *scrollTimer;
    QTimer *timeTimer;
    QTimer *bottomScrollTimer;

    QString announcementText;
    int scrollPosition;
    QString notificationText;
    int bottomScrollPosition;
};

#endif // MAINWINDOW_H
