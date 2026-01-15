#ifndef NETWORKWORKER_H
#define NETWORKWORKER_H

#include <QObject>
#include <QTcpSocket> // 新增
#include <QTimer>
#include <QSqlDatabase>

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
    void onReceiveTimeout();     // 接收超时

private:
    void updateLocalDb(const QByteArray &jsonData);
    void saveSchedules(const QJsonArray &array);
    void saveClassrooms(const QJsonArray &array);
    void saveAnnouncements(const QJsonArray &array);

    QSqlDatabase getDatabase();

    QTcpSocket *socket;
    QTimer *retryTimer;
    QTimer *receiveTimer;
    QByteArray buffer;
    qint32 expectedDataSize;
    bool receivingData;
};

#endif // NETWORKWORKER_H
