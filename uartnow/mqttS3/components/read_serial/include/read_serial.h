#ifndef READ_SERIAL_H
#define READ_SERIAL_H
#include <string.h>
// uint16_t data_read;

#define STILL_CONNECTED_MSG         "slave_KEEP_connect"
#define STILL_CONNECTED_MSG_SIZE    (sizeof(STILL_CONNECTED_MSG))
#define BUTTON_MSG      "BUTTON_MSG"
#define REQUEST_CONNECTION_MSG      "KEEP_connect"
#define RESPONSE_AGREE      "AGREE_connect"

typedef struct {
    char message[20];
    uint8_t mac[6] ;
} connect_request;

void uart_config(void);
void uart_event_task(void);
void add_json(void);
void dump_uart(uint8_t *message, size_t len);
int get_data(uint8_t *data);
uint8_t wait_connect_serial();
void delay(int x);
#endif