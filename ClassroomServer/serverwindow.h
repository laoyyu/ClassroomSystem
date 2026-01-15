#ifndef SERVERWINDOW_H
#define SERVERWINDOW_H

#include <QSqlDatabase>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTextEdit>
#include <QWidget>

class ServerWindow : public QWidget
{
    Q_OBJECT

public:
    ServerWindow(QWidget *parent = nullptr);
    ~ServerWindow();

private slots:
    void onNewConnection();
    void onReadClientData();

private:
    void initDb();                // 初始化服务端数据库
    void initSampleData();        // 初始化示例数据
    QByteArray getScheduleJson(); // 从数据库获取数据并转为JSON

    QTcpServer *tcpServer;
    QTextEdit *logViewer;
    QSqlDatabase db;
};

#endif // SERVERWINDOW_H
