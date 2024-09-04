#ifndef SLAVE_ESPNOW_PROTOCOL_H
#define SLAVE_ESPNOW_PROTOCOL_H

#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <assert.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/timers.h"
#include "nvs_flash.h"
#include "esp_random.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_now.h"
#include "esp_crc.h"
#include "esp_timer.h"
#include "driver/temperature_sensor.h"
#include "read_serial.h"
/* ESPNOW can work in both station and softap mode. It is configured in menuconfig. */
#if CONFIG_ESPNOW_WIFI_MODE_STATION
#define ESPNOW_WIFI_MODE WIFI_MODE_STA
#define ESPNOW_WIFI_IF   ESP_IF_WIFI_STA
#else
#define ESPNOW_WIFI_MODE WIFI_MODE_AP
#define ESPNOW_WIFI_IF   ESP_IF_WIFI_AP
#endif

/* WiFi configuration that you can set via the project configuration menu */
#if CONFIG_ESP_WPA3_SAE_PWE_HUNT_AND_PECK
#define ESP_WIFI_SAE_MODE WPA3_SAE_PWE_HUNT_AND_PECK
#define MASTER_H2E_IDENTIFIER ""
#elif CONFIG_ESP_WPA3_SAE_PWE_HASH_TO_ELEMENT
#define ESP_WIFI_SAE_MODE WPA3_SAE_PWE_HASH_TO_ELEMENT
#define MASTER_H2E_IDENTIFIER CONFIG_ESP_WIFI_PW_ID
#elif CONFIG_ESP_WPA3_SAE_PWE_BOTH
#define ESP_WIFI_SAE_MODE WPA3_SAE_PWE_BOTH
#define MASTER_H2E_IDENTIFIER CONFIG_ESP_WIFI_PW_ID
#endif
#if CONFIG_ESP_WIFI_AUTH_OPEN
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_OPEN
#elif CONFIG_ESP_WIFI_AUTH_WEP
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WEP
#elif CONFIG_ESP_WIFI_AUTH_WPA_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA2_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA2_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA_WPA2_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA_WPA2_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA3_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA3_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA2_WPA3_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA2_WPA3_PSK
#elif CONFIG_ESP_WIFI_AUTH_WAPI_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WAPI_PSK
#endif

#define TAG                         "ESPNOW_SLAVE"
#define REQUEST_CONNECTION_MSG      "slave_REQUEST_connect"
#define MASTER_AGREE_CONNECT_MSG    "master_AGREE_connect"
#define SLAVE_SAVED_MAC_MSG         "slave_SAVED_mac"
#define CHECK_CONNECTION_MSG        "master_CHECK_connect"
#define STILL_CONNECTED_MSG         "slave_KEEP_connect"
#define WIFI_CONNECTED_BIT          BIT0
#define WIFI_FAIL_BIT               BIT1
#define SLAVE_BROADCAST_MAC         { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF }
#define ESPNOW_QUEUE_SIZE           6
#define CURRENT_INDEX               0
#define MAX_DATA_LEN                250
#define IS_BROADCAST_ADDR(addr)     (memcmp(addr, s_slave_broadcast_mac, ESP_NOW_ETH_ALEN) == 0)

typedef struct {
    uint8_t peer_addr[ESP_NOW_ETH_ALEN];
    bool connected;                            
    TickType_t start_time;
    TickType_t end_time;
} mac_master_t;

typedef enum {
    SLAVE_ESPNOW_SEND_CB,
    SLAVE_ESPNOW_RECV_CB,
} slave_espnow_event_id_t;

typedef struct {
    uint8_t mac_addr[ESP_NOW_ETH_ALEN];
    esp_now_send_status_t status;
} slave_espnow_event_send_cb_t;

typedef struct {
    uint8_t mac_addr[ESP_NOW_ETH_ALEN];
    uint8_t data[MAX_DATA_LEN];
    int data_len;
} slave_espnow_event_recv_cb_t;

typedef union {
    slave_espnow_event_send_cb_t send_cb;
    slave_espnow_event_recv_cb_t recv_cb;
} slave_espnow_event_info_t;

/* When ESPNOW sending or receiving callback function is called, post event to ESPNOW task. */
typedef struct {
    slave_espnow_event_id_t id;
    slave_espnow_event_info_t info;
} slave_espnow_event_t;

/* Parameters of sending ESPNOW data. */
typedef struct {
    bool unicast;                         //Send unicast ESPNOW data.
    bool broadcast;                       //Send broadcast ESPNOW data.
    uint8_t state;                        //Indicate that if has received broadcast ESPNOW data or not.
    uint32_t magic;                       //Magic number which is used to determine which device to send unicast ESPNOW data.
    uint16_t count;                       //Total count of unicast ESPNOW data to be sent.
    uint16_t delay;                       //Delay between sending two ESPNOW data, unit: ms.
    int len;                              //Length of ESPNOW data to be sent, unit: byte.
    uint8_t buffer[MAX_DATA_LEN];                      //Buffer pointing to ESPNOW data.
    uint8_t dest_mac[ESP_NOW_ETH_ALEN];   //MAC address of destination device.
} slave_espnow_send_param_t;



typedef struct {
    uint8_t mac[6];
    float temperature_mcu;
    signed rssi;
    float temperature_rdo;
    float do_value;
    float temperature_phg;
    float ph_value;
} sensor_data_t;


void init_temperature_sensor();
float read_internal_temperature_sensor(void);

void event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);
void slave_wifi_init(void);

void erase_peer(const uint8_t *peer_mac);
void add_peer(const uint8_t *peer_mac, bool encrypt); 
esp_err_t response_specified_mac(const uint8_t *dest_mac, const char *message, bool encrypt);
void slave_espnow_send_cb(const uint8_t *mac_addr, esp_now_send_status_t status);
void slave_espnow_recv_cb(const esp_now_recv_info_t *recv_info, const uint8_t *data, int len);
void slave_espnow_task(void *pvParameter);
esp_err_t slave_espnow_init(void);
void slave_espnow_deinit(slave_espnow_send_param_t *send_param);
void slave_espnow_protocol();
char* json_data();
void prepare_data(sensor_data_t *data);

#endif //SLAVE_ESPNOW_PROTOCOL_H