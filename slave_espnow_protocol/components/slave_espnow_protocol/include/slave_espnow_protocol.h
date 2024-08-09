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

/* ESPNOW can work in both station and softap mode. It is configured in menuconfig. */
#if CONFIG_ESPNOW_WIFI_MODE_STATION
#define ESPNOW_WIFI_MODE WIFI_MODE_STA
#define ESPNOW_WIFI_IF   ESP_IF_WIFI_STA
#else
#define ESPNOW_WIFI_MODE WIFI_MODE_AP
#define ESPNOW_WIFI_IF   ESP_IF_WIFI_AP
#endif

#define TAG                         "Espnow_slave"
#define REQUEST_CONNECTION_MSG      "Slave request connection"
#define MASTER_AGREE_CONNECT_MSG    "Master agree to connect"
#define SLAVE_SAVED_MAC_MSG         "Slave saved MAC Master"
#define CHECK_CONNECTION_MSG        "Master request to check connection"
#define STILL_CONNECTED_MSG         "Slave still keeps the connection"
#define SLAVE_BROADCAST_MAC         { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF }
#define EMPTY_MAC_ADDR              { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }
#define ESPNOW_QUEUE_SIZE           6
#define TIMEOUT_CHECK_CONNECT       150000
#define IS_BROADCAST_ADDR(addr)     (memcmp(addr, s_slave_broadcast_mac, ESP_NOW_ETH_ALEN) == 0)

typedef struct {
    TickType_t start_time;
    TickType_t end_time;
    uint8_t peer_addr[ESP_NOW_ETH_ALEN];
    bool connected;                            
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
    uint8_t *data;
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
    uint8_t *buffer;                      //Buffer pointing to ESPNOW data.
    uint8_t dest_mac[ESP_NOW_ETH_ALEN];   //MAC address of destination device.
} slave_espnow_send_param_t;

void add_peer(const uint8_t *peer_mac);
void slave_espnow_send_cb(const uint8_t *mac_addr, esp_now_send_status_t status);
esp_err_t response_specified_mac(const uint8_t *dest_mac, const char *message);
void slave_espnow_recv_cb(const esp_now_recv_info_t *recv_info, const uint8_t *data, int len);
void slave_wifi_init(void);
void slave_espnow_task(void *pvParameter);
esp_err_t slave_espnow_init(void);
void slave_espnow_deinit(slave_espnow_send_param_t *send_param);

#endif //SLAVE_ESPNOW_PROTOCOL_H