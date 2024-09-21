#ifndef READ_SERIAL_H
#define READ_SERIAL_H
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#define STILL_CONNECTED_MSG         "slave_KEEP_connect"
#define STILL_CONNECTED_MSG_SIZE    (sizeof(STILL_CONNECTED_MSG))
#define BUTTON_MSG      "BUTTON_MSG"
#define REQUEST_CONNECTION_MSG      "CONNECT_request"
#define GET_DATA      "GET_DATA"
#define GET_FULL_DATA      "GET_FULL_DATA"
#define SET_RELAY_STATE      "SET_RELAY_STATE"
#define WAKE_UP_COMMAND     "WAKE_UP"
#define MAX_SLAVES                  3
#define WOKE_UP   "WOKE_UP"

#define RESPONSE_AGREE      "AGREE_connect"
#define RESPONSE_CONNECTED      "CONNECTED"
#define REQUEST_UART "CONNECT_request"
#define MESSAGES_DATA "KEEP_connect"


typedef struct {
    char message[20];
    uint8_t mac[6];
} connect_request;

typedef struct {
    char message[20];
} messages_request;

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
    bool relay_state;

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

// uint8_t mac_massss[6] = {0x34, 0x85, 0x18, 0x25, 0x2d, 0x94};
extern table_device_t table_devices[MAX_SLAVES];

void uart_config(void);
void uart_event_task(void);
void add_json(void);
void dump_uart(uint8_t *message, size_t len);
int get_uart(uint8_t *message, size_t len, int timeout);
int get_data(float *data1, float *data2, float *data3, float *data4);
void wait_connect_serial();
int wait_wake_up();
void get_table();
void delay(int x);
void accept_connect(uint8_t *message);
void log_table_devices();
void parse_payload(const sensor_data_t *espnow_data);
#endif