#ifndef READ_SERIAL_H
#define READ_SERIAL_H


void uart_config(void);
void uart_event_task(void);
void add_json(void);
void dump_uart(const char *message);



#endif