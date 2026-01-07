#ifndef NETWORKWORKER_H
#define NETWORKWORKER_H

#include <QObject>
#include <QTcpSocket> // 新增
#include <QTimer>

class NetworkWorker : public QObject
{
    Q_OBJECT
public:
    explicit NetworkWorker(QObject *parent = nullptr);

public slots:
    void startSync();

signals:
    void dataUpdated(const QString &msg);
    void announcementUpdated(const QString &title, const QString &content);

private slots:
    void connectToServer();      // 连接服务器
    void onConnected();          // 连接成功
    void onReadyRead();          // 读取数据
    void onError(QAbstractSocket::SocketError socketError); // 错误处理

private:
    void updateLocalDb(const QByteArray &jsonData);
    void saveSchedules(const QJsonArray &array); // 解析JSON并存库

    QTcpSocket *socket;
    QTimer *retryTimer; // 用于断线重连或定时刷新
    QByteArray buffer;
};

#endif // NETWORKWORKER_H
