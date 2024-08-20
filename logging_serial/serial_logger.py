import serial
import csv
import time

# Cấu hình cổng COM
COM_PORT = 'COM24'  # Thay đổi theo cổng COM của bạn
BAUD_RATE = 115200  # Tốc độ baud rate, thay đổi nếu cần
TIMEOUT = 1  # Thời gian chờ

# Đường dẫn file CSV
CSV_FILE = 'data_log_multi_slave_farm_v4.csv'

def main():
    # Mở cổng COM
    ser = serial.Serial(COM_PORT, BAUD_RATE, timeout=TIMEOUT)

    # Mở file CSV để ghi
    with open(CSV_FILE, mode='w', newline='') as csvfile:
        writer = csv.writer(csvfile)
        writer.writerow(['Timestamp', 'Data'])  # Ghi tiêu đề cột

        try:
            while True:
                # Đọc dữ liệu từ cổng COM
                data = ser.readline().decode('utf-8').strip()

                if data:
                    # Ghi dữ liệu vào file CSV
                    timestamp = time.strftime('%Y-%m-%d %H:%M:%S')
                    writer.writerow([timestamp, data])
                    print(f"{timestamp}: {data}")
        except KeyboardInterrupt:
            # Xử lý khi nhấn Ctrl+C để dừng chương trình
            print("Đã dừng ghi dữ liệu.")
        finally:
            # Đóng cổng COM và file CSV
            ser.close()

if __name__ == '__main__':
    main()
