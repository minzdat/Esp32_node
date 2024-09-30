import os
import psutil
import serial
import serial.tools.list_ports

# Danh sách lưu các cổng COM đang mở
open_ports = []

# Hàm liệt kê các cổng COM khả dụng
def get_available_com_ports():
    ports = serial.tools.list_ports.comports()
    return [port.device for port in ports]

# Hàm hiển thị các cổng COM khả dụng
def display_available_com_ports():
    available_ports = get_available_com_ports()
    if available_ports:
        print("Available COM ports:")
        for port in available_ports:
            print(f"- {port}")
    else:
        print("No available COM ports.")
        
# Hàm mở cổng COM mới (hỗ trợ nhiều cổng COM và kiểm tra cổng có tồn tại)
def open_com_ports(port_names, baud_rate=115200, log_to_file=False):
    available_ports = get_available_com_ports()
    ports_to_open = [port.strip() for port in port_names.split(",") if port.strip()]

    for port_name in ports_to_open:
        if port_name not in available_ports:
            print(f"{port_name} is not available.")
            continue
        if port_name not in open_ports:
            open_ports.append(port_name)
            # Mở một terminal mới để đọc dữ liệu từ cổng COM
            os.system(f"start cmd /k python serial_read.py {port_name} {baud_rate} {log_to_file}")
            print(f"Opened {port_name} at baud rate {baud_rate} successfully.")
        else:
            print(f"{port_name} is already open.")

# Hàm đóng nhiều cổng COM chỉ định
def close_com_ports(port_names):
    ports_to_close = [port.strip() for port in port_names.split(",") if port.strip()]
    for port_name in ports_to_close:
        close_com_port(port_name)
        
# Hàm đóng cổng COM chỉ định
def close_com_port(port_name):
    if port_name in open_ports:
        open_ports.remove(port_name)
        # Tìm và đóng cửa sổ cmd liên quan đến cổng COM
        for proc in psutil.process_iter():
            try:
                # Kiểm tra xem quá trình có tên là "cmd.exe" đang chạy hay không
                if proc.name() == "cmd.exe":
                    cmd_line = proc.cmdline()
                    if f"serial_read.py {port_name}" in ' '.join(cmd_line):
                        proc.kill()
                        print(f"Closed {port_name}.")
                        return
            except (psutil.NoSuchProcess, psutil.AccessDenied):
                continue
        print(f"Could not find a process for {port_name}.")
    else:
        print(f"{port_name} is not open.")

# Hàm đóng tất cả các cổng COM
def close_all_ports():
    for port in open_ports.copy():
        close_com_port(port)
    print("Closed all COM ports.")

# Hàm hiển thị các cổng COM đang mở
def show_open_ports():
    if open_ports:
        print("Open COM ports:")
        for port in open_ports:
            print(f"- {port}")
    else:
        print("No COM ports are currently open.")

# Chương trình chính
def main():
    while True:
        print("")
        print("_"*50)
        print("Options:")
        print("1. Open COM port(s)")
        print("2. Close specific COM port(s)")
        print("3. Close all COM ports")
        print("4. Show open COM ports")
        print("5. Exit")
        print("-"*50)

        choice = input("Enter your choice: ")

        if choice == "1":
            display_available_com_ports()
            port_names = input("Enter COM port(s) to open (e.g., COM3, COM5): ")
            baud_rate = input("Enter baud rate (default is 115200): ")
            log_choice = input("Do you want to log to file? (True/False): ")
            
            if not baud_rate.strip():
                baud_rate = 115200  # Sử dụng mặc định nếu người dùng không nhập gì
            else:
                baud_rate = int(baud_rate)
                
            open_com_ports(port_names, baud_rate, log_choice.lower() == 'true')
        elif choice == "2":
            ports = input("Enter COM port(s) to close (e.g., COM3, COM4): ")
            close_com_ports(ports)
        elif choice == "3":
            close_all_ports()
        elif choice == "4":
            show_open_ports()
        elif choice == "5":
            close_all_ports()  # Đóng tất cả cổng khi thoát
            print("Exiting...")
            break
        else:
            print("Invalid choice. Please try again.")

if __name__ == "__main__":
    main()