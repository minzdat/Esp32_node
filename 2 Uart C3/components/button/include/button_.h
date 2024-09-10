#ifndef BUTTON_H
#define BUTTON_H

#include "iot_button.h"


#define CONTROL_KEY_GPIO      GPIO_NUM_0

static void button_single_cb(void *arg, void *usr_data);
static void button_double_cb(void *arg, void *usr_data);
static void button_longpress_cb(void *arg, void *usr_data);
void button_init();



#endif // UART_H