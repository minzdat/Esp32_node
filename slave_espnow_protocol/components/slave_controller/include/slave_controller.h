#ifndef SLAVE_CONTROLLER_H
#define SLAVE_CONTROLLER_H

#include <stdio.h>
#include "esp_log.h"
#include "driver/gpio.h"
#include "driver/rtc_io.h"
#include "light_sleep.h"

#define TAG_SLAVE_CONTROLLER            "SLAVE_CONTROLLER"
#define GPIO_RELAY_PIN                  12
#define GPIO_LED_CONNECT                20  
#define GPIO_LED                        21 

typedef enum {
    LED_CONNECT,
    LED
} led_id_t;

typedef enum {
    DEVICE_RELAY,
    DISCONNECT_NODE,
    DEVICE_LED_CONNECT,
    DEVICE_LED,
    DEVICE_FLOAT,
    DEVICE_UNKNOWN
} device_type_t;

extern bool relay_state;

void relay_init(void);
void relay_on(void);
void relay_off(void);

void led_init(void);
void led_control(led_id_t led_id, bool state);

void handle_device(device_type_t device_type, bool state); 
void slave_controller_init();

#endif //SLAVE_CONTROLLER_H