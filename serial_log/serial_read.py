import serial
import sys
import csv
import re
import os
from datetime import datetime

# Hàm để loại bỏ mã màu ANSI
def remove_ansi_escape_codes(text):
    ansi_escape = re.compile(r'\x1B\[[0-?9;]*[mK]')
    return ansi_escape.sub('', text)

# Hàm đọc dữ liệu từ cổng COM và hiển thị log
def read_from_com(port_name, baud_rate, log_to_file):
    try:
        # Mở cổng COM với tốc độ baud nhận từ tham số
        ser = serial.Serial(port_name, baudrate=baud_rate, timeout=1)
        
        # Tắt tín hiệu DTR và RTS để tránh gửi tín hiệu điều khiển đến ESP32
        ser.dtr = False
        ser.rts = False
        
        print(f"Opened {port_name} at baud rate {baud_rate} successfully.")
        
        log_filename = None
        log_writer = None
        
        # Tạo thư mục nếu chưa tồn tại
        log_directory = "files_serial_log"
        if log_to_file:
            if not os.path.exists(log_directory):
                os.makedirs(log_directory)
                print(f"Created directory: {log_directory}")
            
            # Tạo tên file CSV với thời gian hiện tại
            log_filename = os.path.join(log_directory, f"serial_log_{datetime.now().strftime('%Y%m%d_%H%M%S')}.csv")
        
            # Mở file CSV để ghi log
            log_file = open(log_filename, mode='w', newline='')
            log_writer = csv.writer(log_file)
            log_writer.writerow(["Timestamp", "Log"])  # Ghi tiêu đề cột
            
        while True:
            # Chỉ đọc dữ liệu từ cổng COM
            line = ser.readline().decode('utf-8', errors='ignore').strip()
            if line:
                # Loại bỏ mã màu ANSI
                clean_line = remove_ansi_escape_codes(line)
                
                # In ra log trên console
                print(f"[{port_name}] {line}")
                
                # Ghi log vào file CSV nếu được chọn
                if log_writer:
                    log_writer.writerow([datetime.now().strftime('%Y-%m-%d %H:%M:%S'), clean_line])
                    
    except serial.SerialException as e:
        print(f"Could not open {port_name}: {e}")
    except KeyboardInterrupt:
        ser.close()
        if log_file:
            log_file.close()
        print(f"Closed {port_name}")
        sys.exit()
        
if __name__ == "__main__":
    if len(sys.argv) != 4:
        print("Usage: python serial_read.py <COM_PORT> <BAUD_RATE> <LOG_TO_FILE (True/False)>")
        sys.exit(1)

    port_name = sys.argv[1]
    baud_rate = int(sys.argv[2])
    log_to_file = sys.argv[3].lower() == 'true'  # Chuyển đổi sang boolean
    read_from_com(port_name, baud_rate, log_to_file)
