#ifndef BUTTON_H
#define BUTTON_H

#include "iot_button.h"
#include "uart.h"


#define CONTROL_KEY_GPIO      GPIO_NUM_0
// #define CONFIG_BUTTON_LONG_PRESS_TIME_MS 1800
// #define CONFIG_BUTTON_SHORT_PRESS_TIME_MS 1800 

static void button_single_cb(void *arg, void *usr_data);
static void button_double_cb(void *arg, void *usr_data);
static void button_longpress_cb(void *arg, void *usr_data);
void button_init();



#endif // UART_H