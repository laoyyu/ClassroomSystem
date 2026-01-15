import sqlite3
from datetime import datetime, timedelta
import os

# 使用程序实际运行时的数据库路径
db_path = 'build/Desktop_Qt_6_9_2_MSVC2022_64bit-Debug/server_data.db'

# 如果 build 目录不存在，使用当前目录
if not os.path.exists(os.path.dirname(db_path)):
    db_path = 'server_data.db'
    print(f'警告: build 目录不存在，使用当前目录: {db_path}')
else:
    print(f'使用 build 目录数据库: {db_path}')

conn = sqlite3.connect(db_path)
cursor = conn.cursor()

cursor.execute('''
    CREATE TABLE IF NOT EXISTS master_schedules (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        room TEXT,
        course TEXT,
        teacher TEXT,
        time_slot TEXT,
        start_time TEXT,
        end_time TEXT,
        weekday INTEGER,
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

import random

courses = [
    ('高等数学', '王教授'),
    ('大学英语', '李老师'),
    ('大学物理', '陈工'),
    ('线性代数', '张教授'),
    ('计算机基础', '刘老师'),
    ('数据结构', '赵教授'),
    ('有机化学', '孙老师'),
    ('分析化学', '周教授'),
    ('大学语文', '吴老师'),
    ('写作与表达', '郑老师'),
    ('概率论', '冯教授'),
    ('数理统计', '陈老师'),
    ('电路原理', '杨教授'),
    ('模拟电子技术', '朱老师'),
    ('操作系统', '钱教授'),
    ('计算机网络', '林老师'),
    ('数据库系统', '梁教授'),
    ('软件工程', '宋老师'),
    ('工程力学', '何教授'),
    ('材料力学', '高老师'),
    ('管理学原理', '马老师'),
    ('市场营销', '罗老师'),
    ('人工智能导论', '唐教授'),
    ('机器学习', '许老师'),
    ('数字信号处理', '韩教授'),
    ('通信原理', '邓老师'),
    ('宏观经济学', '曹老师'),
    ('微观经济学', '彭老师'),
    ('法理学', '魏老师'),
    ('宪法学', '谢老师'),
    ('离散数学', '黄教授'),
    ('算法设计', '胡教授'),
    ('编译原理', '郭教授'),
    ('计算机图形学', '董老师'),
    ('移动开发', '田老师'),
    ('网络安全', '范教授'),
    ('数据挖掘', '姜教授'),
    ('云计算', '夏教授'),
    ('深度学习', '傅教授'),
    ('物联网技术', '钟教授'),
]

classrooms = ['Class 101', 'Class 102', 'Class 103', 'Class 104', 'Class 105', 
           'Class 201', 'Class 202', 'Class 203', 'Class 204', 'Class 205', 
           'Class 301', 'Class 302', 'Class 303', 'Class 304', 'Class 305']

time_slots = [
    ('08:00', '09:40', '08:00 - 09:40'),
    ('10:00', '11:40', '10:00 - 11:40'),
    ('14:00', '15:40', '14:00 - 15:40'),
    ('16:00', '17:40', '16:00 - 17:40'),
    ('19:00', '20:40', '19:00 - 20:40')
]

# 创建一个占用矩阵，记录每个教室每周每时段的占用情况
# 结构：{(room, weekday, start_time, end_time): (course, teacher)}
occupancy = {}

# 为每个教室分配一周的课程
all_schedule_data = []

for day in range(1, 8):  # 周一到周日
    for room in classrooms:
        # 每个教室每天最多安排3节课
        available_slots = time_slots.copy()
        random.shuffle(available_slots)
        
        # 随机选择几个时段安排课程
        num_classes_per_day = random.randint(2, 3)  # 每天2-3节课
        selected_slots = available_slots[:num_classes_per_day]
        
        for start_time, end_time, time_display in selected_slots:
            # 检查这个时间段是否已被占用
            if (room, day, start_time, end_time) not in occupancy:
                # 从课程库中随机选择一门课
                course, teacher = random.choice(courses)
                
                # 添加到占用矩阵
                occupancy[(room, day, start_time, end_time)] = (course, teacher)
                
                # 添加到数据列表
                # (room, course, teacher, time_slot, start_time, end_time, weekday, is_next)
                all_schedule_data.append((room, course, teacher, time_display, start_time, end_time, day, 0))

# 执行插入操作
cursor.executemany('INSERT INTO master_schedules VALUES (?, ?, ?, ?, ?, ?, ?, ?)', all_schedule_data)

# 重新定义 now 变量用于公告部分
current_time = datetime.now()

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
    ('系统通知', '欢迎使用智慧教室班牌系统！本系统提供课程信息查询、教室状态显示等功能。', 1, current_time.strftime('%Y-%m-%d %H:%M:%S'), (current_time + timedelta(days=30)).strftime('%Y-%m-%d %H:%M:%S')),
    ('考试安排', '期末考试将于下周一开始，请同学们提前做好准备。', 2, current_time.strftime('%Y-%m-%d %H:%M:%S'), (current_time + timedelta(days=7)).strftime('%Y-%m-%d %H:%M:%S')),
    ('维护通知', '系统将于本周六凌晨2:00-4:00进行维护升级，期间服务可能中断。', 0, current_time.strftime('%Y-%m-%d %H:%M:%S'), (current_time + timedelta(days=2)).strftime('%Y-%m-%d %H:%M:%S'))
]

cursor.executemany('INSERT INTO announcements (title, content, priority, publish_time, expire_time) VALUES (?, ?, ?, ?, ?)', announcement_data)

conn.commit()

cursor.execute('SELECT * FROM master_schedules')
rows = cursor.fetchall()
print('数据库已初始化，插入的数据：')
for row in rows:
    print(row)

# 关闭第一个连接
conn.close()
print(f'\n数据库文件已创建: {db_path}')

# 同时更新根目录下的数据库文件
db_root_path = 'server_data.db'
conn2 = sqlite3.connect(db_root_path)
cursor2 = conn2.cursor()

# 创建表结构（如果不存在）
cursor2.execute('''
    CREATE TABLE IF NOT EXISTS master_schedules (
        room TEXT,
        course TEXT,
        teacher TEXT,
        time_slot TEXT,
        start_time TEXT,
        end_time TEXT,
        weekday INTEGER,
        is_next INTEGER
    )
''')

cursor2.execute('''
    CREATE TABLE IF NOT EXISTS classrooms (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        room_name TEXT UNIQUE,
        class_name TEXT,
        capacity INTEGER,
        building TEXT,
        floor INTEGER
    )
''')

cursor2.execute('''
    CREATE TABLE IF NOT EXISTS announcements (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        title TEXT,
        content TEXT,
        priority INTEGER DEFAULT 0,
        publish_time TEXT,
        expire_time TEXT
    )
''')

cursor2.execute('''
    CREATE TABLE IF NOT EXISTS sync_log (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        sync_time TEXT,
        status TEXT,
        items_count INTEGER
    )
''')

# 检查并更新表结构（如果需要）
cursor2.execute('PRAGMA table_info(master_schedules)')
cols = [row[1] for row in cursor2.fetchall()]

# 如果没有id字段，则重建表结构
if 'id' not in cols:
    # 重建表结构
    cursor2.execute('ALTER TABLE master_schedules RENAME TO temp_master_schedules')
    
    # 创建新表结构
    cursor2.execute('''
        CREATE TABLE master_schedules (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            room TEXT,
            course TEXT,
            teacher TEXT,
            time_slot TEXT,
            start_time TEXT,
            end_time TEXT,
            weekday INTEGER,
            is_next INTEGER
        )
    ''')
    
    # 复制数据
    cursor2.execute('''
        INSERT INTO master_schedules (room, course, teacher, time_slot, start_time, end_time, weekday, is_next)
        SELECT room, course, teacher, time_slot, start_time, end_time, weekday, is_next FROM temp_master_schedules
    ''')
    
    # 删除旧表
    cursor2.execute('DROP TABLE temp_master_schedules')

# 清空已有数据
cursor2.execute('DELETE FROM master_schedules')
cursor2.execute('DELETE FROM classrooms')
cursor2.execute('DELETE FROM announcements')

# 插入新的数据
cursor2.executemany('INSERT INTO master_schedules (room, course, teacher, time_slot, start_time, end_time, weekday, is_next) VALUES (?, ?, ?, ?, ?, ?, ?, ?)', all_schedule_data)
cursor2.executemany('INSERT INTO classrooms (room_name, class_name, capacity, building, floor) VALUES (?, ?, ?, ?, ?)', classroom_data)
cursor2.executemany('INSERT INTO announcements (title, content, priority, publish_time, expire_time) VALUES (?, ?, ?, ?, ?)', announcement_data)

conn2.commit()
conn2.close()
print(f'根目录数据库文件也已更新: {db_root_path}')
