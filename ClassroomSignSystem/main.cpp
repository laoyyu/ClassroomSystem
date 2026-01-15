#include "mainwindow.h"
#include "DatabaseManager.h"
#include <QApplication>
#include <QDebug>
#include <QSqlDatabase>

int main(int argc, char *argv[])
{
    qDebug() << "程序启动...";

    QApplication a(argc, argv);

    qDebug() << "QApplication创建成功";

    qDebug() << "可用的数据库驱动:" << QSqlDatabase::drivers();

    // 设置样式表美化一下
    a.setStyleSheet(R"(
        QWidget { font-family: 'Segoe UI', Arial; }
        QGroupBox { font-weight: bold; border: 1px solid gray; border-radius: 5px; margin-top: 10px; }
        QGroupBox::title { subcontrol-origin: margin; left: 10px; padding: 0 3px; }
        QLineEdit { padding: 5px; border: 1px solid #ccc; border-radius: 3px; }
        QHeaderView::section { background-color: #f0f0f0; padding: 4px; border: 1px solid #ddd; }
    )");

    qDebug() << "开始创建MainWindow...";
    MainWindow w;
    qDebug() << "MainWindow创建成功";

    DatabaseManager::initSampleData();

    w.setWindowTitle("智慧教室班牌系统");
    w.show();

    qDebug() << "开始事件循环...";
    return a.exec();
}
