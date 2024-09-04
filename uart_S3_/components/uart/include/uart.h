/* UART Echo Example
---MASTER S3---GATEWAY
   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#ifndef UART_H
#define UART_H

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "sdkconfig.h"
#include "esp_log.h"
#include "cJSON.h"
#include <esp_mac.h>
#include "esp_crc.h"
/*
 * - Port: configured UART
 * - Receive (Rx) buffer: on
 * - Transmit (Tx) buffer: off
 * - Flow control: off
 * - Event queue: off
 * - Pin assignment: see defines below (See Kconfig)
 */

#define UART0_NUM          UART_NUM_0 //defauft - flash
#define UART1_NUM          UART_NUM_1
#define UART_BAUD_RATE     (115200)
#define TASK_STACK_SIZE    (1024*2)
#define QUEUE_SIZE         (20)

#define TXD1_PIN           (5)
#define RXD1_PIN           (4)
#define RTS1_PIN           (9)
#define CTS1_PIN           (8)

#define PATTERN_CHR_NUM    (3)         /*!< Set the number of consecutive and identical characters received by receiver which defines a UART pattern*/

#define BUF_SIZE (1024*4)
#define RD_BUF_SIZE        (BUF_SIZE)

#define MAX_DEVICES 10 //device


extern QueueHandle_t uart1_queue;

/*Struct data---------------------------------*/
typedef struct __attribute__((packed)) {
    uint8_t mac[6];
    float temperature_mcu;
    int rssi;
    float temperature_rdo;
    float do_value;
    float temperature_phg;
    float ph_value;
} sensor_data_t; //slave_sensor_data
/**/
typedef struct __attribute__((packed)){
    //uint8_t mac[6];
    sensor_data_t data;  // 
    char message[10];  // 
    uint16_t crc; 
} sensor_data_uart_t;
typedef struct {
    uint8_t mac[6];
    sensor_data_t data;
    // Thêm các thông tin khác nếu cần
} device_t; //device_id

extern int num_devices;
extern device_t devices[];

void process_uart_data(uint8_t *data, size_t length);
void send_data_to_mqtt(int device_index, void (*data_to_mqtt_func)(const char*, const char*, int, int));
void datatomqtt1();
void datatomqtt2();
int sendData(const char* logName, const char* data);
void tx1_task(void *arg);
void rx1_task(void *arg);
int find_device_index(uint8_t mac[6]);
void update_or_add_device(sensor_data_t *new_data);
void log_all_devices();
void uart_init(void);
void uart_event_task(void *pvParameters);

#endif // UART_H