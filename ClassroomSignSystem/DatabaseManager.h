#ifndef DATABASEMANAGER_H
#define DATABASEMANAGER_H

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include <QStandardPaths>
#include <QDir>
#include <QDateTime>

class DatabaseManager {
public:
    static bool initDb() {
        qDebug() << "开始初始化数据库...";

        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
        QString dbPath = "classroom.db";
        db.setDatabaseName(dbPath);

        qDebug() << "数据库路径:" << dbPath;

        if (!db.open()) {
            qDebug() << "DB Error: " << db.lastError();
            return false;
        }

        qDebug() << "数据库打开成功";

        QSqlQuery query;

        qDebug() << "开始创建表...";

        if (!query.exec("CREATE TABLE IF NOT EXISTS schedules ("
                   "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                   "room_name TEXT, "
                   "course_name TEXT, "
                   "teacher TEXT, "
                   "time_slot TEXT, "
                   "start_time TEXT, "
                   "end_time TEXT, "
                   "weekday INTEGER, "
                   "is_next INTEGER DEFAULT 0)")) {
            qDebug() << "创建schedules表失败:" << query.lastError();
        } else {
            qDebug() << "schedules表创建成功";
        }

        if (!query.exec("CREATE TABLE IF NOT EXISTS classrooms ("
                   "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                   "room_name TEXT UNIQUE, "
                   "class_name TEXT, "
                   "capacity INTEGER, "
                   "building TEXT, "
                   "floor INTEGER)")) {
            qDebug() << "创建classrooms表失败:" << query.lastError();
        } else {
            qDebug() << "classrooms表创建成功";
        }

        if (!query.exec("CREATE TABLE IF NOT EXISTS announcements ("
                   "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                   "title TEXT, "
                   "content TEXT, "
                   "priority INTEGER DEFAULT 0, "
                   "publish_time TEXT, "
                   "expire_time TEXT)")) {
            qDebug() << "创建announcements表失败:" << query.lastError();
        } else {
            qDebug() << "announcements表创建成功";
        }

        if (!query.exec("CREATE TABLE IF NOT EXISTS sync_log ("
                   "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                   "sync_time TEXT, "
                   "status TEXT, "
                   "items_count INTEGER)")) {
            qDebug() << "创建sync_log表失败:" << query.lastError();
        } else {
            qDebug() << "sync_log表创建成功";
        }

        qDebug() << "数据库初始化完成";

        return true;
    }

    static bool clearAllTables() {
        QSqlQuery query;
        query.exec("DELETE FROM schedules");
        query.exec("DELETE FROM classrooms");
        query.exec("DELETE FROM announcements");
        query.exec("DELETE FROM sync_log");
        qDebug() << "已清空所有表数据";
        return true;
    }

    static bool initSampleData() {
        QSqlQuery query;

        query.exec("SELECT COUNT(*) FROM schedules");
        if (query.next() && query.value(0).toInt() > 0) {
            return true;
        }

        qDebug() << "Initializing sample data...";

        query.exec("INSERT INTO schedules (room_name, course_name, teacher, time_slot, is_next) VALUES "
                   "('Class 101', '高等数学', '张教授', '08:00-09:40', 0), "
                   "('Class 101', '大学英语', '李老师', '10:00-11:40', 1), "
                   "('Class 102', '数据结构', '王教授', '08:00-09:40', 0), "
                   "('Class 102', '操作系统', '赵老师', '10:00-11:40', 1), "
                   "('Class 103', '计算机网络', '刘教授', '08:00-09:40', 0), "
                   "('Class 103', '数据库原理', '陈老师', '10:00-11:40', 1)");

        query.exec("INSERT INTO classrooms (room_name, class_name, capacity, building, floor) VALUES "
                   "('Class 101', '计算机科学2023级1班', 50, 'A栋', 3), "
                   "('Class 102', '计算机科学2023级2班', 45, 'A栋', 3), "
                   "('Class 103', '软件工程2023级1班', 60, 'B栋', 2)");

        query.exec("INSERT INTO announcements (title, content, priority, publish_time, expire_time) VALUES "
                   "('系统通知', '欢迎使用智慧教室班牌系统！本系统提供课程信息查询、教室状态显示等功能。', 1, "
                   "'" + QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss") + "', "
                   "'" + QDateTime::currentDateTime().addDays(30).toString("yyyy-MM-dd HH:mm:ss") + "')");

        qDebug() << "Sample data initialized successfully";
        return true;
    }

    static bool insertAnnouncement(const QString &title, const QString &content, int priority = 0) {
        QSqlQuery query;
        query.prepare("INSERT INTO announcements (title, content, priority, publish_time, expire_time) "
                     "VALUES (?, ?, ?, ?, ?)");
        query.addBindValue(title);
        query.addBindValue(content);
        query.addBindValue(priority);
        query.addBindValue(QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss"));
        query.addBindValue(QDateTime::currentDateTime().addDays(7).toString("yyyy-MM-dd HH:mm:ss"));
        return query.exec();
    }

    static bool insertClassroom(const QString &roomName, const QString &className, int capacity, const QString &building, int floor) {
        QSqlQuery query;
        query.prepare("INSERT OR REPLACE INTO classrooms (room_name, class_name, capacity, building, floor) "
                     "VALUES (?, ?, ?, ?, ?)");
        query.addBindValue(roomName);
        query.addBindValue(className);
        query.addBindValue(capacity);
        query.addBindValue(building);
        query.addBindValue(floor);
        return query.exec();
    }

    static bool logSync(const QString &status, int itemsCount) {
        QSqlQuery query;
        query.prepare("INSERT INTO sync_log (sync_time, status, items_count) VALUES (?, ?, ?)");
        query.addBindValue(QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss"));
        query.addBindValue(status);
        query.addBindValue(itemsCount);
        return query.exec();
    }
};

#endif // DATABASEMANAGER_H
