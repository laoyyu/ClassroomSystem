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

class MainWindow : public QWidget
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void updateDisplay();
    void onDataSynced(const QString &msg);
    void onAnnouncementUpdated(const QString &title, const QString &content);
    void filterData(const QString &text);
    void updateCurrentTime();
    void scrollAnnouncement();
    void loadAnnouncement();

private:
    void setupUi();
    void setupModel();
    void startWorker();

    QLabel *lblCourseName;
    QLabel *lblTeacher;
    QLabel *lblTime;
    QLabel *lblNextCourse;
    QLabel *lblStatus;
    QLabel *lblAnnouncement;
    QLabel *lblCurrentTime;

    QLineEdit *searchBox;
    QTableView *tableView;
    QTableView *classroomView;

    QSqlTableModel *model;
    QSqlTableModel *classroomModel;

    QThread *workerThread;
    QTimer *scrollTimer;
    QTimer *timeTimer;

    QString announcementText;
    int scrollPosition;
};

#endif // MAINWINDOW_H
