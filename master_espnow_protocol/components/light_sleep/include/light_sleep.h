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
#include "esp_sleep.h"
#include "master_espnow_protocol.h"

#define TIMER_WAKEUP_TIME_US    (2 * 1000 * 1000)

void light_sleep_mode();
esp_err_t register_timer_wakeup(void);

#endif //LIGHT_SLEEP_H