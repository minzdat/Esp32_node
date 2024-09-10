#ifndef READ_SERIAL_H
#define READ_SERIAL_H
<<<<<<< HEAD
#include <string.h>
#include <stdlib.h>
#include "esp_wifi.h"
// uint16_t data_read;

#define STILL_CONNECTED_MSG         "slave_KEEP_connect"
#define STILL_CONNECTED_MSG_SIZE    (sizeof(STILL_CONNECTED_MSG))
#define BUTTON_MSG      "BUTTON_MSG"
#define REQUEST_CONNECTION_MSG      "CONNECT_request"

#define RESPONSE_AGREE      "AGREE_connect"
#define RESPONSE_CONNECTED      "CONNECTED"
#define REQUEST_UART "KEEP_connect"

typedef struct {
    char message[20];
    uint8_t mac[6] ;
} connect_request;

typedef struct {
    uint8_t mac[6];
    char message[20];
    uint8_t type;
    uint8_t crc;
} frame_request;

// typedef struct {
//     float temperature_mcu;
//     int rssi;
//     float temperature_rdo;
//     float do_value;
//     float temperature_phg;
//     float ph_value;
//     // uint16_t crc;
//     char message[STILL_CONNECTED_MSG_SIZE];
// } sensor_data_t;

typedef struct {
    float temperature_mcu;
    int rssi;
    float temperature_rdo;
    float do_value;
    float temperature_phg;
    float ph_value;
} sensor_data_t;
typedef struct {
    uint8_t peer_addr[6];    // ESPNOW peer MAC address
    bool status;                            // Variable status has two statuses online: 1 and offline: 0
    sensor_data_t data;                     // Data devices
} table_device_t;
typedef struct {
    uint8_t type;                         //[1 bytes] Broadcast or unicast ESPNOW data.
    uint16_t seq_num;                     //[2 bytes] Sequence number of ESPNOW data.
    uint16_t crc;         
    char message[20];                 //[2 bytes] CRC16 value of ESPNOW data.
    // uint8_t payload[120];     //[120 bytes] Real payload of ESPNOW data.
    sensor_data_t payload;
} __attribute__((packed)) espnow_data_t;
=======

>>>>>>> 0d65c9acca272f1193113c0af4e2c3e13a3f601f

void uart_config(void);
void uart_event_task(void);
void add_json(void);
<<<<<<< HEAD
void dump_uart(uint8_t *message, size_t len);
int get_data(float *data1, float *data2, float *data3, float *data4);
uint8_t wait_connect_serial();
void delay(int x);
void accept_connect(uint8_t *message);
=======
void dump_uart(const char *message);



>>>>>>> 0d65c9acca272f1193113c0af4e2c3e13a3f601f
#endif