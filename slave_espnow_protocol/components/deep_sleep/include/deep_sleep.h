#ifndef DEEP_SLEEP_H
#define DEEP_SLEEP_H

#include "esp_timer.h"
#include "esp_sleep.h"
#include "driver/rtc_io.h"

#define WAKEUP_GPIO_PIN             2
#define ENABLE_TIMER_WAKEUP         2    // Unit s

// void deep_sleep_register_gpio_wakeup();
void deep_sleep_register_rtc_timer_wakeup();
void deep_sleep_mode();

#endif //DEEP_SLEEP_H