#ifndef READ_SERIAL_H
#define READ_SERIAL_H

#include <string.h>
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "freertos/queue.h"
#include "driver/uart.h"
#include "driver/gpio.h"

#include "esp_timer.h"
#include "cJSON.h"

#include "mbedtls/aes.h"

// #include "master_espnow_protocol.h"

#include "master_controller.h"

#define TAG_READ_SERIAL                 "READ_SERIAL"

// #define REQUEST_UART                    "KEEP_connect"
#define REQUEST_UART                    "CONNECT_request"
#define RESPONSE_AGREE                  "AGREE_connect"
#define RESPONSE_CONNECTED              "CONNECTED"
#define REQUEST_CONNECTION_MSG          "CONNECT_request"
#define WOKE_UP                         "WOKE_UP"
#define WAKE_UP_COMMAND                 "WAKE_UP"

#define BUTTON_MSG                      "BUTTON_MSG"
#define GET_DATA                        "GET_DATA"
#define GET_FULL_DATA                   "GET_FULL_DATA"

// #define PATTERN_CHR_NUM                 (3)                  /*!< Set the number of consecutive and identical characters received by receiver which defines a UART pattern*/
#define UART_NUM_P2                     UART_NUM_1              // Sử dụng UART1
#define TX_GPIO_NUM                     5                       // Chân TX (thay đổi nếu cần)
#define RX_GPIO_NUM                     4                       // Chân RX (thay đổi nếu cần)
#define BAUD_RATE                       115200                  // Tốc độ baud
#define BUF_SIZE                        (1024)
#define RD_BUF_SIZE                     (BUF_SIZE)
#define MAX_SLAVES                      3

// uint8_t reponse_connect_uart[20];

typedef struct {
    // uint8_t mac[6];
    float temperature_mcu;
    int rssi;
    float temperature_rdo;
    float do_value;
    float temperature_phg;
    float ph_value;
    bool relay_state;

} sensor_data_tt;

// typedef struct {
//     float temperature_mcu;
//     int rssi;
//     float temperature_rdo;
//     float do_value;
//     float temperature_phg;
//     float ph_value;
// } sensor_data_t;

typedef struct {
    uint8_t peer_addr[6];                           // ESPNOW peer MAC address
    bool status;                                    // Variable status has two statuses online: 1 and offline: 0
    sensor_data_tt data;                            // Data devices
} table_device_tt;

typedef struct {
    uint8_t type;                                   //[1 bytes] Broadcast or unicast ESPNOW data.
    uint16_t seq_num;                               //[2 bytes] Sequence number of ESPNOW data.
    uint16_t crc;                                   //[2 bytes] CRC16 value of ESPNOW data.
    uint8_t payload[120];                           //Real payload of ESPNOW data.
} __attribute__((packed)) espnow_data_tt;

typedef struct {
    char message[20];
    uint8_t mac[6] ;
} messages_request;

// extern table_device_tt table_devices[MAX_SLAVES];

void uart_config(void);
void encrypt_message(const unsigned char *input, unsigned char *output, size_t length);
void dump_uart(uint8_t *message, size_t len);
void add_json();
void uart_event(void *pvParameters);
void delay(int x);
void check_timeout();
void uart_event_task(void);
void accept_connect(uint8_t *message);
uint8_t wait_connect_serial();

#endif // READ_SERIAL_H