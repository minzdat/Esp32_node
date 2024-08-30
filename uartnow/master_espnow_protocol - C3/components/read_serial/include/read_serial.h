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

void uart_config(void);
void uart_event_task(void);
void add_json(void);
void dump_uart(sensor_data_tt *message);



#endif