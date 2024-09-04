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


static QueueHandle_t uart1_queue;

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
typedef struct {
    uint8_t mac[6];
    sensor_data_t data;
    // Thêm các thông tin khác nếu cần
} device_t; //device_id

extern int num_devices;
extern device_t devices[];

void uart_init(void);
void uart_read_task(void *param);
int find_device_index(uint8_t mac[6]);
void update_or_add_device(sensor_data_t *new_data);
void process_uart_data(uint8_t *data, size_t length);
void log_all_devices();
static void uart_event_task(void *pvParameters);
static void tx1_task(void *arg);
static void rx1_task(void *arg);

#endif // UART_H