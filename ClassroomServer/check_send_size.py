import sqlite3
import json

conn = sqlite3.connect('server_data.db')
cursor = conn.cursor()

# 模拟服务端构造 JSON 的逻辑
root_obj = {}

# schedules
cursor.execute('SELECT room, course, teacher, time_slot, is_next FROM master_schedules')
schedules = []
for row in cursor.fetchall():
    schedules.append({
        'room_name': row[0],
        'course_name': row[1],
        'teacher': row[2],
        'time_slot': row[3],
        'is_next': row[4]
    })
root_obj['schedules'] = schedules

# classrooms
cursor.execute('SELECT room_name, class_name, capacity, building, floor FROM classrooms')
classrooms = []
for row in cursor.fetchall():
    classrooms.append({
        'room_name': row[0],
        'class_name': row[1],
        'capacity': row[2],
        'building': row[3],
        'floor': row[4]
    })
root_obj['classrooms'] = classrooms

# announcements
cursor.execute('SELECT title, content, priority, publish_time, expire_time FROM announcements')
announcements = []
for row in cursor.fetchall():
    announcements.append({
        'title': row[0],
        'content': row[1],
        'priority': row[2],
        'publish_time': row[3],
        'expire_time': row[4]
    })
root_obj['announcements'] = announcements

conn.close()

json_data = json.dumps(root_obj, ensure_ascii=False).encode('utf-8')
print(f'服务端将发送的 JSON 数据大小: {len(json_data)} 字节')
print(f'schedules 数量: {len(schedules)}')
print(f'classrooms 数量: {len(classrooms)}')
print(f'announcements 数量: {len(announcements)}')
print(f'\nJSON 前 500 字符:')
print(json_data[:500].decode('utf-8', errors='ignore'))
