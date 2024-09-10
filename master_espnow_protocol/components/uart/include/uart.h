#ifndef READ_SERIAL_H
#define READ_SERIAL_H
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "driver/gpio.h"

#define UART1_NUM         UART_NUM_1    // Sử dụng UART1
#define UART_BAUD_RATE     (115200)
#define TASK_STACK_SIZE    (1024*2)
#define QUEUE_SIZE         (20)
#define TXD1_PIN           (5)    // Chân TX (thay đổi nếu cần)
#define RXD1_PIN           (4)    // Chân RX (thay đổi nếu cần)

#define PATTERN_CHR_NUM    (3)  

extern int count;
extern int count1;
extern int count_num;
extern QueueHandle_t uart1_queue;

#define BUF_SIZE (1024*4)
#define RD_BUF_SIZE        (BUF_SIZE)
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
typedef enum {
    EVENT_TYPE_A,
    EVENT_TYPE_B,
} custom_event_t;

void uart_event_task(void *pvParameters);
void uart_config(void);
int sendStructData(const char* logName, const void* data, size_t size);
void send_uart(sensor_data_tt *data, const char *message);
void send_uart_(int UART_NUM, const char *_message);

#endif