import socket
import struct
import json
import sqlite3
import os

print("=" * 60)
print("手动测试客户端-服务端数据同步")
print("=" * 60)

# 1. 连接服务端
print("\n[1] 连接服务端 127.0.0.1:12345...")
sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
try:
    sock.connect(('127.0.0.1', 12345))
    print("✓ 连接成功")
except Exception as e:
    print(f"✗ 连接失败: {e}")
    exit(1)

# 2. 发送请求
print("\n[2] 发送同步请求...")
sock.sendall(b"GET_SCHEDULE")
print("✓ 请求已发送")

# 3. 接收数据大小（4字节大端）
print("\n[3] 接收数据长度头...")
size_header = sock.recv(4)
if len(size_header) != 4:
    print(f"✗ 长度头接收失败，只收到 {len(size_header)} 字节")
    exit(1)

data_size = struct.unpack('>I', size_header)[0]
print(f"✓ 预期数据大小: {data_size} 字节")

# 4. 接收完整 JSON 数据
print("\n[4] 接收 JSON 数据...")
buffer = b""
while len(buffer) < data_size:
    chunk = sock.recv(min(8192, data_size - len(buffer)))
    if not chunk:
        print(f"✗ 连接断开，只收到 {len(buffer)} 字节")
        break
    buffer += chunk
    print(f"  已接收: {len(buffer)} / {data_size} 字节")

sock.close()

if len(buffer) != data_size:
    print(f"✗ 数据接收不完整")
    exit(1)

print(f"✓ 数据接收完成，总计 {len(buffer)} 字节")

# 5. 解析 JSON
print("\n[5] 解析 JSON...")
try:
    data = json.loads(buffer.decode('utf-8'))
    print(f"✓ JSON 解析成功")
    print(f"  - schedules 数量: {len(data.get('schedules', []))}")
    print(f"  - classrooms 数量: {len(data.get('classrooms', []))}")
    print(f"  - announcements 数量: {len(data.get('announcements', []))}")
except Exception as e:
    print(f"✗ JSON 解析失败: {e}")
    exit(1)

# 6. 显示前 5 条课程
print("\n[6] 前5条课程数据:")
for i, schedule in enumerate(data.get('schedules', [])[:5]):
    print(f"  {i+1}. {schedule['room_name']} | {schedule['course_name']} | {schedule['teacher']}")

# 7. 保存到客户端数据库
db_path = 'ClassroomSignSystem/build/Desktop_Qt_6_9_2_MSVC2022_64bit-Debug/classroom.db'
if not os.path.exists(db_path):
    print(f"\n[7] 客户端数据库文件不存在: {db_path}")
    print("跳过数据库写入测试")
else:
    print(f"\n[7] 更新客户端数据库: {db_path}")
    conn = sqlite3.connect(db_path)
    cursor = conn.cursor()
    
    # DROP + CREATE schedules
    cursor.execute("DROP TABLE IF EXISTS schedules")
    cursor.execute("""CREATE TABLE schedules (
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        room_name TEXT,
        course_name TEXT,
        teacher TEXT,
        time_slot TEXT,
        is_next INTEGER DEFAULT 0)""")
    
    # 插入数据
    for schedule in data.get('schedules', []):
        cursor.execute(
            "INSERT INTO schedules (room_name, course_name, teacher, time_slot, is_next) VALUES (?, ?, ?, ?, ?)",
            (schedule['room_name'], schedule['course_name'], schedule['teacher'], 
             schedule['time_slot'], schedule['is_next'])
        )
    
    conn.commit()
    
    # 验证
    cursor.execute("SELECT COUNT(*) FROM schedules")
    count = cursor.fetchone()[0]
    print(f"✓ 已写入 {count} 条课程记录")
    
    cursor.execute("SELECT room_name, course_name, teacher FROM schedules LIMIT 3")
    print("\n数据库前3条记录:")
    for row in cursor.fetchall():
        print(f"  {row[0]} | {row[1]} | {row[2]}")
    
    conn.close()

print("\n" + "=" * 60)
print("测试完成！如果上面显示30条课程，说明服务端数据正常。")
print("请重新编译并运行客户端程序，应该能看到所有数据。")
print("=" * 60)
