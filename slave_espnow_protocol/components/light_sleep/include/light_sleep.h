#ifndef LIGHT_SLEEP_H
#define LIGHT_SLEEP_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "esp_sleep.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_check.h"
#include "driver/gpio.h"
#include "slave_espnow_protocol.h"

#define TAG_LIGHT_SLEEP         "LIGHT_SLEEP"
#define TIMER_WAKEUP_TIME_US    (2 * 1000 * 1000)
#define TIMER_LIGHT_SLEEP       (1 * 1000 * 1000)    // Unit us

/* Most development boards have "boot" button attached to GPIO0.
 * You can also change this to another pin.
 */
#if CONFIG_IDF_TARGET_ESP32C3 || CONFIG_IDF_TARGET_ESP32C2 || CONFIG_IDF_TARGET_ESP32H2 \
    || CONFIG_IDF_TARGET_ESP32C6
#define BOOT_BUTTON_NUM         9
#elif CONFIG_IDF_TARGET_ESP32C5
#define BOOT_BUTTON_NUM         28
#elif CONFIG_IDF_TARGET_ESP32P4
#define BOOT_BUTTON_NUM         35
#else
#define BOOT_BUTTON_NUM         0
#endif

/* Use boot button as gpio input */
#define GPIO_WAKEUP_NUM         BOOT_BUTTON_NUM
/* "Boot" button is active low */
#define GPIO_WAKEUP_LEVEL       0

extern bool light_sleep_flag;
extern int64_t start_time_light_sleep;

esp_err_t register_timer_wakeup(void);
void wait_gpio_inactive(void);
esp_err_t register_gpio_wakeup(void);
void light_sleep_task(void *args);

#endif //LIGHT_SLEEP_H