#ifndef SLEEP_H
#define SLEEP_H

#include <time.h>
//#include "esp_check.h"
#include "esp_sleep.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "driver/uart.h"  
#include "driver/gpio.h"   

esp_err_t register_timer_wakeup(int timer_sleep_in_s);
esp_err_t register_uart_wakeup(uart_port_t uart_num);
//esp_err_t register_gpio_wakeup(gpio_num_t gpio_num, int level);
void sleep_init(int sleep_time, bool uart_wakeup, uart_port_t uart_num);

void go_to_sleep(void);
//void calculateRunningTime(void (*func)(void), char *func_name);

#endif // SLEEP_H
