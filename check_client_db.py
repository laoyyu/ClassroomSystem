import sqlite3
import os

os.chdir('ClassroomSignSystem')

if not os.path.exists('classroom.db'):
    print("客户端数据库文件不存在！")
    exit(1)

conn = sqlite3.connect('classroom.db')
cursor = conn.cursor()

cursor.execute('SELECT COUNT(*) FROM schedules')
count = cursor.fetchone()[0]
print(f'客户端课程表记录数: {count}')

cursor.execute('SELECT * FROM schedules LIMIT 10')
print('\n前10条记录:')
for row in cursor.fetchall():
    print(row)

conn.close()
