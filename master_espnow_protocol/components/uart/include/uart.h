#ifndef READ_SERIAL_H
#define READ_SERIAL_H
#include <string.h>

typedef struct __attribute__((packed)){
    uint8_t mac[6];
    float temperature_mcu;
    int rssi;
    float temperature_rdo;
    float do_value;
    float temperature_phg;
    float ph_value;
} sensor_data_tt;
typedef struct __attribute__((packed)){
    //uint8_t mac[6];
    sensor_data_tt data;  // Nhúng cấu trúc nhận vào cấu trúc gửi
    char message[10];  // Thêm trường cho thông điệp
    uint16_t crc; 
} sensor_data_uart_t;

static void uart_event_task(void *pvParameters);
void uart_config(void);
int sendStructData(const char* logName, const void* data, size_t size);
void send_uart(sensor_data_tt *data, const char *message);


#endif