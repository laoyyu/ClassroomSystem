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
        QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
        QString dbPath = "classroom.db";
        db.setDatabaseName(dbPath);

        if (!db.open()) {
            qDebug() << "DB Error: " << db.lastError();
            return false;
        }

        QSqlQuery query;

        query.exec("CREATE TABLE IF NOT EXISTS schedules ("
                   "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                   "room_name TEXT, "
                   "course_name TEXT, "
                   "teacher TEXT, "
                   "time_slot TEXT, "
                   "is_next INTEGER DEFAULT 0)");

        query.exec("CREATE TABLE IF NOT EXISTS classrooms ("
                   "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                   "room_name TEXT UNIQUE, "
                   "class_name TEXT, "
                   "capacity INTEGER, "
                   "building TEXT, "
                   "floor INTEGER)");

        query.exec("CREATE TABLE IF NOT EXISTS announcements ("
                   "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                   "title TEXT, "
                   "content TEXT, "
                   "priority INTEGER DEFAULT 0, "
                   "publish_time TEXT, "
                   "expire_time TEXT)");

        query.exec("CREATE TABLE IF NOT EXISTS sync_log ("
                   "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                   "sync_time TEXT, "
                   "status TEXT, "
                   "items_count INTEGER)");

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
