import sqlite3

# 打开两个数据库
server_conn = sqlite3.connect('../ClassroomServer/server_data.db')
client_conn = sqlite3.connect('build/Desktop_Qt_6_9_2_MSVC2022_64bit-Debug/classroom.db')

server_cursor = server_conn.cursor()
client_cursor = client_conn.cursor()

# 从服务端读取数据
server_cursor.execute('SELECT room, course, teacher, time_slot, is_next FROM master_schedules')
schedules = server_cursor.fetchall()

# 清空客户端表
client_cursor.execute('DELETE FROM schedules')

# 插入到客户端（注意字段名不同）
for row in schedules:
    client_cursor.execute(
        'INSERT INTO schedules (room_name, course_name, teacher, time_slot, is_next) VALUES (?, ?, ?, ?, ?)',
        row
    )

client_conn.commit()
print(f'成功同步 {len(schedules)} 条课程记录')

# 同理处理教室和公告
server_cursor.execute('SELECT room_name, class_name, capacity, building, floor FROM classrooms')
classrooms = server_cursor.fetchall()
client_cursor.execute('DELETE FROM classrooms')
for row in classrooms:
    client_cursor.execute(
        'INSERT INTO classrooms (room_name, class_name, capacity, building, floor) VALUES (?, ?, ?, ?, ?)',
        row
    )
client_conn.commit()
print(f'成功同步 {len(classrooms)} 条教室记录')

server_cursor.execute('SELECT title, content, priority, publish_time, expire_time FROM announcements')
announcements = server_cursor.fetchall()
client_cursor.execute('DELETE FROM announcements')
for row in announcements:
    client_cursor.execute(
        'INSERT INTO announcements (title, content, priority, publish_time, expire_time) VALUES (?, ?, ?, ?, ?)',
        row
    )
client_conn.commit()
print(f'成功同步 {len(announcements)} 条公告记录')

server_conn.close()
client_conn.close()
print('\n数据同步完成！请重启客户端程序查看。')
