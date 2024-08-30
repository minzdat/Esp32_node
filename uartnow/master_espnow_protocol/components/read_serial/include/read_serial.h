#ifndef READ_SERIAL_H
#define READ_SERIAL_H
#include <string.h>

typedef struct {
    uint8_t mac[6];
    float temperature_mcu;
    int rssi;
    float temperature_rdo;
    float do_value;
    float temperature_phg;
    float ph_value;
} sensor_data_tt;
typedef struct {
    uint8_t type;                         //[1 bytes] Broadcast or unicast ESPNOW data.
    uint16_t seq_num;                     //[2 bytes] Sequence number of ESPNOW data.
    uint16_t crc;                         //[2 bytes] CRC16 value of ESPNOW data.
    uint8_t payload[120];                  //Real payload of ESPNOW data.
} __attribute__((packed)) espnow_data_tt;

typedef struct {
    char message[20];
    uint8_t mac[6] ;
} messages_request;

#define REQUEST_UART "KEEP_connect"
#define REQUEST_UART "KEEP_connect"
// uint8_t reponse_connect_uart[20];
#define RESPONSE_AGREE      "AGREE_connect"
#define BUTTON_MSG      "BUTTON_MSG"

void uart_config(void);
void uart_event_task(void);
void add_json(void);
void dump_uart(uint8_t *message, size_t len);
uint8_t wait_connect_serial();
void check_timeout();

#endif