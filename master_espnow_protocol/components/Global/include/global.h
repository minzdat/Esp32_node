#ifndef GLOBAL_H
#define GLOBAL_H

// DEFINE BOARD BEFORE FLASHING
// #define BOARD_ESP32_S3_6_RELAY
// #define BOARD_ESP32_S3
#define BOARD_FARM_V1


#include <stdio.h>
#include "version.h"
#include <stdint.h>
#include <stddef.h>
#include <inttypes.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <ctype.h>
#include <sys/time.h>
#include <sys/param.h>
#include "driver/uart.h"
#include "driver/gpio.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "freertos/ringbuf.h"
#include "freertos/event_groups.h"

#include "esp_log.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_attr.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "esp_wifi.h"
#include "esp_ota_ops.h"
#include "esp_http_client.h"
#include "esp_https_ota.h"
#include "esp_crt_bundle.h"
#include "esp_sntp.h"
#include "esp_task_wdt.h"
#include "esp_netif_sntp.h"
#include "esp_mac.h"
#include "esp_timer.h"
#include "esp_tls.h"
#include "esp_pm.h"

#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"
#include "lwip/ip_addr.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "mqtt_client.h"
#include "cJSON.h"

#include "nvs.h"
#include "nvs_flash.h"
#include "driver/temperature_sensor.h"

// Define for OTA
#define OTA_URL_SIZE                            256
#define BUFFER_OTA_SIZE                         1024
#define CONFIG_EXAMPLE_SKIP_VERSION_CHECK

// Define for Wifi
#define CONFIG_EXAMPLE_WIFI_CONN_MAX_RETRY      100

#ifdef BOARD_ESP32_S3_6_RELAY

    #define UART_NUM                                UART_NUM_1
    #define TXD_PIN                                 GPIO_NUM_17
    #define RXD_PIN                                 GPIO_NUM_18
    #define GPIO_INPUT_IO_WATER                     1
    #define GPIO_INPUT_IO_AIR                       2
    #define GPIO_INPUT_IO_RDO                       4
    #define GPIO_INPUT_IO_RS485                     6
    #define GPIO_INPUT_IO_SIM                       21
    #define GPIO_INPUT_IO_BOUY                      42

#endif

#ifdef BOARD_ESP32_S3

    #define UART_NUM                                UART_NUM_1
    #define TXD_PIN                                 GPIO_NUM_17
    #define RXD_PIN                                 GPIO_NUM_18
    #define GPIO_INPUT_IO_WATER                     1
    #define GPIO_INPUT_IO_AIR                       2
    #define GPIO_INPUT_IO_RDO                       4
    #define GPIO_INPUT_IO_RS485                     6
    #define GPIO_INPUT_IO_SIM                       21
    #define GPIO_INPUT_IO_BOUY                      42

#endif

#ifdef BOARD_FARM_V1

    #define UART_NUM                                UART_NUM_2
    #define TXD_PIN                                 GPIO_NUM_15
    #define RXD_PIN                                 GPIO_NUM_16
    #define GPIO_INPUT_IO_WATER                     1
    #define GPIO_INPUT_IO_AIR                       2
    #define GPIO_INPUT_IO_RDO                       4
    #define GPIO_INPUT_IO_RS485                     6
    #define GPIO_INPUT_IO_SIM                       21
    #define GPIO_INPUT_IO_BOUY                      42

#endif

#define BAUD_RATE                               9600
#define BUF_SIZE                                1024
// #define TIMEOUT_THRESHOLD                       2

#define DATA_ERROR                              0

#define CLEANUP_VERSION                         1
#define CONFIG_SNTP_TIME_SERVER                 "pool.ntp.org"
// Event management
#define READY_CLEANER_EVENT                     BIT0
#define READY_MEASUREMENT_EVENT                 BIT1
#define FINISH_CLEANER_EVENT                    BIT2
#define FINISH_MEASUREMENT_EVENT                BIT3
#define SYSTEM_INIT_SUCCESS_EVENT               BIT4
#define SYSTEM_INIT_FAILED_EVENT                BIT5
#define READY_OTA_EVENT                         BIT6
#define START_OTA_EVENT                         BIT7
#define OTA_FAILED_EVENT                        BIT8
#define FINISH_OTA_EVENT                        (BIT7|BIT8)
#define WIFI_CONNECTED_EVENT                    BIT9
#define WIFI_STATE_EVENT                        BIT9
#define READY_ADJUST_EVENT                      BIT10
#define FINISH_ADJUST_EVENT                     BIT11
#define FINISH_PUBLISH_EVENT                    BIT12
#define BOUY_RESET_EVENT                        BIT13
#define BOUY_SET_EVENT                          BIT14
#define BOUY_READY_EVENT                        BIT15
#define EVENT_WAIT                              FINISH_MEASUREMENT_EVENT|FINISH_CLEANER_EVENT|SYSTEM_INIT_SUCCESS_EVENT|SYSTEM_INIT_FAILED_EVENT\
                                                |FINISH_ADJUST_EVENT|BOUY_RESET_EVENT|BOUY_SET_EVENT


#define GPIO_OUTPUT_PIN_SEL  ((1ULL<<GPIO_INPUT_IO_WATER) | (1ULL<<GPIO_INPUT_IO_RDO) | (1ULL<<GPIO_INPUT_IO_RS485) | (1ULL<<GPIO_INPUT_IO_SIM))
#define GPIO_INPUT_PIN_SEL   (1ULL<<GPIO_INPUT_IO_BOUY)

#define WATER_ON()          gpio_set_level(GPIO_INPUT_IO_WATER,1)
#define WATER_OFF()         gpio_set_level(GPIO_INPUT_IO_WATER,0)
#define AIR_ON()            gpio_set_level(GPIO_INPUT_IO_AIR,1)
#define AIR_OFF()           gpio_set_level(GPIO_INPUT_IO_AIR,0)
#define RDO_ON()            gpio_set_level(GPIO_INPUT_IO_RDO,1)
#define RDO_OFF()           gpio_set_level(GPIO_INPUT_IO_RDO,0)
#define RS485_ON()          gpio_set_level(GPIO_INPUT_IO_RS485,1)
#define RS485_OFF()         gpio_set_level(GPIO_INPUT_IO_RS485,0)
#define SIM_ON()            gpio_set_level(GPIO_INPUT_IO_SIM,1)
#define SIM_OFF()           gpio_set_level(GPIO_INPUT_IO_SIM,0)
#define READ_BOUY()         gpio_get_level(GPIO_INPUT_IO_BOUY)

#define CONFIG_LOG_UDP_SERVER_IP "34.92.71.73"

#define UDP_ENABLE             1    // 1: Enable UDP, 0: Disable UDP
typedef struct
{
    double uptime;
    char internet[33]; // SSID max length is 32 + 1 for null terminator
    int rssi;
    char mac[18]; // MAC address string is 17 characters + 1 for null terminator
    char ip_address[16]; // IPv4 address string is 15 characters + 1 for null terminator
    uint8_t bootnum;
    char firmware_version[10]; // Enough to hold "1.0.0" and potential future versions
} device_parameter_t;

extern uint8_t g_restart_counter;
extern char *g_envisor_id;

void get_device_init_parameter(device_parameter_t *device_parameter);
void get_device_frequent_parameter(device_parameter_t *device_parameter);
void temp_cpu_init(uint8_t temp_min, uint8_t temp_max);
void get_cpu_temp(float *cpu_temp);
esp_err_t power_save(void);
int id_to_port(char *client_id);

#endif // GLOBAL_H