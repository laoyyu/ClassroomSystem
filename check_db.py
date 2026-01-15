import sqlite3
import sys

db_path = sys.argv[1] if len(sys.argv) > 1 else 'classroom.db'

try:
    conn = sqlite3.connect(db_path)
    cursor = conn.cursor()
    
    cursor.execute('SELECT COUNT(*) FROM schedules')
    count = cursor.fetchone()[0]
    print(f'课程表记录数: {count}')
    
    cursor.execute('SELECT room_name, course_name, teacher FROM schedules LIMIT 10')
    print('\n前10条记录:')
    for row in cursor.fetchall():
        print(f'  {row[0]} | {row[1]} | {row[2]}')
    
    conn.close()
except Exception as e:
    print(f'错误: {e}')
