#include <QCoreApplication>
#include <QDebug>
#include <QSqlDatabase>

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    qDebug() << "=== 测试程序启动 ===";
    qDebug() << "Qt版本:" << QT_VERSION_STR;

    qDebug() << "\n=== 检查数据库驱动 ===";
    QStringList drivers = QSqlDatabase::drivers();
    qDebug() << "可用的数据库驱动:";
    for (const QString &driver : drivers) {
        qDebug() << "  -" << driver;
    }

    if (!drivers.contains("QSQLITE")) {
        qDebug() << "\n错误: 缺少QSQLITE驱动!";
        return 1;
    }

    qDebug() << "\n=== 测试数据库连接 ===";
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName("test.db");

    if (!db.open()) {
        qDebug() << "错误: 无法打开数据库:" << db.lastError().text();
        return 1;
    }

    qDebug() << "成功: 数据库已打开";
    db.close();

    qDebug() << "\n=== 测试完成 ===";
    return 0;
}