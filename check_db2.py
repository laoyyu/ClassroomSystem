import sqlite3
import sys

db_path = sys.argv[1] if len(sys.argv) > 1 else 'classroom.db'

try:
    conn = sqlite3.connect(db_path)
    cursor = conn.cursor()
    
    # 尝试服务端的表名
    try:
        cursor.execute('SELECT COUNT(*) FROM master_schedules')
        count = cursor.fetchone()[0]
        print(f'[服务端] master_schedules 记录数: {count}')
        
        cursor.execute('SELECT room, course, teacher FROM master_schedules LIMIT 5')
        print('\n前5条记录:')
        for row in cursor.fetchall():
            print(f'  {row[0]} | {row[1]} | {row[2]}')
    except:
        # 尝试客户端的表名
        cursor.execute('SELECT COUNT(*) FROM schedules')
        count = cursor.fetchone()[0]
        print(f'[客户端] schedules 记录数: {count}')
        
        cursor.execute('SELECT room_name, course_name, teacher FROM schedules LIMIT 5')
        print('\n前5条记录:')
        for row in cursor.fetchall():
            print(f'  {row[0]} | {row[1]} | {row[2]}')
    
    conn.close()
except Exception as e:
    print(f'错误: {e}')
