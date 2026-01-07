import sqlite3
from datetime import datetime, timedelta

db_path = 'server_data.db'

conn = sqlite3.connect(db_path)
cursor = conn.cursor()

cursor.execute('''
    CREATE TABLE IF NOT EXISTS master_schedules (
        room TEXT,
        course TEXT,
        teacher TEXT,
        time_slot TEXT,
        is_next INTEGER
    )
''')

cursor.execute('''
    CREATE TABLE IF NOT EXISTS classrooms (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        room_name TEXT UNIQUE,
        class_name TEXT,
        capacity INTEGER,
        building TEXT,
        floor INTEGER
    )
''')

cursor.execute('''
    CREATE TABLE IF NOT EXISTS announcements (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        title TEXT,
        content TEXT,
        priority INTEGER DEFAULT 0,
        publish_time TEXT,
        expire_time TEXT
    )
''')

cursor.execute('''
    CREATE TABLE IF NOT EXISTS sync_log (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        sync_time TEXT,
        status TEXT,
        items_count INTEGER
    )
''')

cursor.execute('DELETE FROM master_schedules')
cursor.execute('DELETE FROM classrooms')
cursor.execute('DELETE FROM announcements')

now = datetime.now()
next_hour = now + timedelta(hours=1)
current_time_slot = f"{now.strftime('%H:%M')} - {next_hour.strftime('%H:%M')}"

data = [
    ('Class 101', '高等数学', '王教授', current_time_slot, 0),
    ('Class 101', '大学英语', '李老师', '14:00 - 15:30', 1),
    ('Class 102', '大学物理', '陈工', '09:00 - 12:00', 0),
    ('Class 102', '线性代数', '张教授', '14:00 - 16:00', 1),
    ('Class 103', '计算机基础', '刘老师', '08:00 - 10:00', 0),
    ('Class 103', '数据结构', '赵教授', '10:30 - 12:30', 1),
    ('Class 104', '有机化学', '孙老师', '09:00 - 11:00', 0),
    ('Class 104', '分析化学', '周教授', '14:00 - 16:00', 1),
    ('Class 105', '大学语文', '吴老师', '08:30 - 10:30', 0),
    ('Class 105', '写作与表达', '郑老师', '11:00 - 12:30', 1),
    ('Class 201', '概率论', '冯教授', '09:00 - 11:00', 0),
    ('Class 201', '数理统计', '陈老师', '14:00 - 16:00', 1),
    ('Class 202', '电路原理', '杨教授', '08:00 - 10:00', 0),
    ('Class 202', '模拟电子技术', '朱老师', '10:30 - 12:30', 1),
    ('Class 203', '操作系统', '钱教授', '09:00 - 11:00', 0),
    ('Class 203', '计算机网络', '林老师', '14:00 - 16:00', 1),
    ('Class 204', '工程力学', '何教授', '08:30 - 10:30', 0),
    ('Class 204', '材料力学', '高老师', '11:00 - 12:30', 1),
    ('Class 205', '管理学原理', '马老师', '09:00 - 11:00', 0),
    ('Class 205', '市场营销', '罗老师', '14:00 - 16:00', 1),
    ('Class 301', '数据库系统', '梁教授', '08:00 - 10:00', 0),
    ('Class 301', '软件工程', '宋老师', '10:30 - 12:30', 1),
    ('Class 302', '人工智能导论', '唐教授', '09:00 - 11:00', 0),
    ('Class 302', '机器学习', '许老师', '14:00 - 16:00', 1),
    ('Class 303', '数字信号处理', '韩教授', '08:30 - 10:30', 0),
    ('Class 303', '通信原理', '邓老师', '11:00 - 12:30', 1),
    ('Class 304', '宏观经济学', '曹老师', '09:00 - 11:00', 0),
    ('Class 304', '微观经济学', '彭老师', '14:00 - 16:00', 1),
    ('Class 305', '法理学', '魏老师', '08:00 - 10:00', 0),
    ('Class 305', '宪法学', '谢老师', '10:30 - 12:30', 1)
]

cursor.executemany('INSERT INTO master_schedules VALUES (?, ?, ?, ?, ?)', data)

classroom_data = [
    ('Class 101', '计算机科学2023级1班', 50, 'A栋', 3),
    ('Class 102', '计算机科学2023级2班', 45, 'A栋', 3),
    ('Class 103', '软件工程2023级1班', 60, 'B栋', 2),
    ('Class 104', '软件工程2023级2班', 55, 'B栋', 2),
    ('Class 105', '人工智能2023级1班', 40, 'C栋', 4),
    ('Class 201', '数据科学2023级1班', 50, 'A栋', 2),
    ('Class 202', '数据科学2023级2班', 45, 'A栋', 2),
    ('Class 203', '网络安全2023级1班', 40, 'B栋', 3),
    ('Class 204', '网络安全2023级2班', 40, 'B栋', 3),
    ('Class 205', '物联网2023级1班', 55, 'C栋', 2),
    ('Class 301', '计算机科学2024级1班', 50, 'A栋', 4),
    ('Class 302', '计算机科学2024级2班', 45, 'A栋', 4),
    ('Class 303', '软件工程2024级1班', 60, 'B栋', 1),
    ('Class 304', '软件工程2024级2班', 55, 'B栋', 1),
    ('Class 305', '人工智能2024级1班', 40, 'C栋', 3)
]

cursor.executemany('INSERT INTO classrooms (room_name, class_name, capacity, building, floor) VALUES (?, ?, ?, ?, ?)', classroom_data)

announcement_data = [
    ('系统通知', '欢迎使用智慧教室班牌系统！本系统提供课程信息查询、教室状态显示等功能。', 1, now.strftime('%Y-%m-%d %H:%M:%S'), (now + timedelta(days=30)).strftime('%Y-%m-%d %H:%M:%S')),
    ('考试安排', '期末考试将于下周一开始，请同学们提前做好准备。', 2, now.strftime('%Y-%m-%d %H:%M:%S'), (now + timedelta(days=7)).strftime('%Y-%m-%d %H:%M:%S')),
    ('维护通知', '系统将于本周六凌晨2:00-4:00进行维护升级，期间服务可能中断。', 0, now.strftime('%Y-%m-%d %H:%M:%S'), (now + timedelta(days=2)).strftime('%Y-%m-%d %H:%M:%S'))
]

cursor.executemany('INSERT INTO announcements (title, content, priority, publish_time, expire_time) VALUES (?, ?, ?, ?, ?)', announcement_data)

conn.commit()

cursor.execute('SELECT * FROM master_schedules')
rows = cursor.fetchall()
print('数据库已初始化，插入的数据：')
for row in rows:
    print(row)

conn.close()
print(f'\n数据库文件已创建: {db_path}')
