#include "MainWindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // 设置样式表美化一下
    a.setStyleSheet(R"(
        QWidget { font-family: 'Segoe UI', Arial; }
        QGroupBox { font-weight: bold; border: 1px solid gray; border-radius: 5px; margin-top: 10px; }
        QGroupBox::title { subcontrol-origin: margin; left: 10px; padding: 0 3px; }
        QLineEdit { padding: 5px; border: 1px solid #ccc; border-radius: 3px; }
        QHeaderView::section { background-color: #f0f0f0; padding: 4px; border: 1px solid #ddd; }
    )");

    MainWindow w;
    w.setWindowTitle("智慧教室班牌系统 v1.0 (Qt6 + MV + DB + Thread)");
    w.show();

    return a.exec();
}
