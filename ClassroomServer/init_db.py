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

cursor.execute('DELETE FROM master_schedules')

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

conn.commit()

cursor.execute('SELECT * FROM master_schedules')
rows = cursor.fetchall()
print('数据库已初始化，插入的数据：')
for row in rows:
    print(row)

conn.close()
print(f'\n数据库文件已创建: {db_path}')
