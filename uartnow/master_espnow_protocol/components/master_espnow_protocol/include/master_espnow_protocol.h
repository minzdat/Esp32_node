#ifndef MASTER_ESPNOW_PROTOCOL_H
#define MASTER_ESPNOW_PROTOCOL_H

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
#include "esp_pm.h"
#include "esp_sleep.h"

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

#define TAG                         "ESPNOW_MASTER"
#define REQUEST_CONNECTION_MSG      "REQUEST_connect"
#define RESPONSE_AGREE_CONNECT      "AGREE_connect"
#define SLAVE_SAVED_MAC_MSG         "SAVED_mac"
#define CHECK_CONNECTION_MSG        "CHECK_connect"
#define STILL_CONNECTED_MSG         "KEEP_connect"
#define NVS_NAMESPACE               "storage"
#define NVS_KEY_SLAVES              "waiting_slaves"
#define WIFI_CONNECTED_BIT          BIT0
#define WIFI_FAIL_BIT               BIT1
#define MASTER_BROADCAST_MAC        { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF }
#define ESPNOW_MAXDELAY             512
#define TIME_CHECK_CONNECT          5000           // Unit ms
#define ENABLE_TIMER_WAKEUP         10 * 1000000    // Unit us
#define RETRY_TIMEOUT               5 * 1000000     // Unit us
#define NUMBER_RETRY                3
#define ESPNOW_QUEUE_SIZE           6
#define MAX_SLAVES                  3
#define CURRENT_INDEX               0
#define MAX_SEND_ERRORS             3
#define MAX_DATA_LEN                250
#define MAX_PAYLOAD_LEN             120
#define STILL_CONNECTED_MSG_SIZE    (sizeof(STILL_CONNECTED_MSG))
#define IS_BROADCAST_ADDR(addr)     (memcmp(addr, s_master_broadcast_mac, ESP_NOW_ETH_ALEN) == 0)
#define DEFAULT_BEACON_TIMEOUT      CONFIG_WIFI_BEACON_TIMEOUT

#if CONFIG_POWER_SAVE_MIN_MODEM
#define DEFAULT_PS_MODE WIFI_PS_MIN_MODEM
#elif CONFIG_POWER_SAVE_MAX_MODEM
#define DEFAULT_PS_MODE WIFI_PS_MAX_MODEM
#elif CONFIG_POWER_SAVE_NONE
#define DEFAULT_PS_MODE WIFI_PS_NONE
#else
#define DEFAULT_PS_MODE WIFI_PS_NONE
#endif /*CONFIG_POWER_SAVE_MODEM*/

typedef struct {
    uint8_t peer_addr[ESP_NOW_ETH_ALEN];    // ESPNOW peer MAC address
    bool status;                            // Variable status has two statuses online: 1 and offline: 0
    int send_errors;                        // Value send error
    TickType_t start_time;
    TickType_t end_time;
    int number_retry;
    int check_connect_errors;
    int count_send;
    int count_receive;
    int count_retry;
} list_slaves_t;

typedef enum {
    MASTER_ESPNOW_SEND_CB,
    MASTER_ESPNOW_RECV_CB,
} master_espnow_event_id_t;

typedef struct {
    uint8_t mac_addr[ESP_NOW_ETH_ALEN];
    esp_now_send_status_t status;
} master_espnow_event_send_cb_t;

typedef struct {
    uint8_t mac_addr[ESP_NOW_ETH_ALEN];
    uint8_t data[MAX_DATA_LEN];
    int data_len;
} master_espnow_event_recv_cb_t;

typedef union {
    master_espnow_event_send_cb_t send_cb;
    master_espnow_event_recv_cb_t recv_cb;
} master_espnow_event_info_t;

/* When ESPNOW sending or receiving callback function is called, post event to ESPNOW task. */
typedef struct {
    master_espnow_event_id_t id;
    master_espnow_event_info_t info;
} master_espnow_event_t;

enum {
    ESPNOW_DATA_BROADCAST,
    ESPNOW_DATA_UNICAST,
    ESPNOW_DATA_MAX,
};

typedef struct {
    float temperature_mcu;
    int rssi;
    float temperature_rdo;
    float do_value;
    float temperature_phg;
    float ph_value;
    // uint16_t crc;
    char message[STILL_CONNECTED_MSG_SIZE];
} sensor_data_t;

typedef struct {
    uint8_t type;                         //[1 bytes] Broadcast or unicast ESPNOW data.
    uint16_t seq_num;                     //[2 bytes] Sequence number of ESPNOW data.
    uint16_t crc;                         //[2 bytes] CRC16 value of ESPNOW data.
    uint8_t payload[MAX_PAYLOAD_LEN];     //[120 bytes] Real payload of ESPNOW data.
} __attribute__((packed)) espnow_data_t;

/* Parameters of sending ESPNOW data. */
typedef struct {
    bool unicast;                         //Send unicast ESPNOW data.
    bool broadcast;                       //Send broadcast ESPNOW data.
    uint8_t state;                        //Indicate that if has received broadcast ESPNOW data or not.
    uint32_t magic;                       //Magic number which is used to determine which device to send unicast ESPNOW data.
    uint16_t count;                       //Total count of unicast ESPNOW data to be sent.
    uint16_t delay;                       //Delay between sending two ESPNOW data, unit: ms.
    int len;                              //Length of ESPNOW data to be sent, unit: byte.
    uint8_t buffer[MAX_DATA_LEN];         //Buffer pointing to ESPNOW data.
    uint8_t dest_mac[ESP_NOW_ETH_ALEN];   //MAC address of destination device.
} master_espnow_send_param_t;

// Function to read temperature internal esp
void init_temperature_sensor();
float read_internal_temperature_sensor(void);

// Function to NVS
void test_allowed_connect_slaves_to_nvs(list_slaves_t *allowed_connect_slaves);
void print_info_slaves(list_slaves_t *info_slaves);
void save_info_slaves_to_nvs(const char *key, list_slaves_t *info_slaves);
void load_info_slaves_from_nvs(const char *key, list_slaves_t *info_slaves);
void erase_key_in_nvs(const char *key);
void erase_all_in_nvs();

// Function to wifi
void event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);
void master_wifi_init(void);

// Function to master espnow
void prepare_payload(espnow_data_t *espnow_data, float temperature_mcu, int rssi, float temperature_rdo, float do_value, float temperature_phg, float ph_value, const char *message);
void parse_payload(const espnow_data_t *espnow_data);
void espnow_data_prepare(master_espnow_send_param_t *send_param, const char *message);
void espnow_data_parse(uint8_t *data, uint16_t data_len);
void add_peer(const uint8_t *peer_mac, bool encrypt); 
void add_waiting_connect_slaves(const uint8_t *mac_addr);
esp_err_t response_specified_mac(const uint8_t *dest_mac, const char *message, bool encrypt);
void master_espnow_send_cb(const uint8_t *mac_addr, esp_now_send_status_t status);
void master_espnow_recv_cb(const esp_now_recv_info_t *recv_info, const uint8_t *data, int len);
void master_espnow_task(void *pvParameter);
void retry_connect_lost_task(void *pvParameter);
esp_err_t master_espnow_init(void);
void master_espnow_deinit(master_espnow_send_param_t *send_param);
void master_espnow_protocol();

#endif //MASTER_ESPNOW_PROTOCOL_H