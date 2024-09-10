#ifndef SLAVE_ESPNOW_PROTOCOL_H
#define SLAVE_ESPNOW_PROTOCOL_H

<<<<<<< HEAD
#include <math.h>
=======
>>>>>>> 0d65c9acca272f1193113c0af4e2c3e13a3f601f
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
<<<<<<< HEAD
#include "deep_sleep.h"
#include "light_sleep.h"
=======
>>>>>>> 0d65c9acca272f1193113c0af4e2c3e13a3f601f

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
<<<<<<< HEAD
#define REQUEST_CONNECTION_MSG      "REQUEST_connect"
#define MASTER_AGREE_CONNECT_MSG    "AGREE_connect"
#define SLAVE_SAVED_MAC_MSG         "SAVED_mac"
#define CHECK_CONNECTION_MSG        "CHECK_connect"
#define STILL_CONNECTED_MSG         "KEEP_connect"
#define NVS_NAMESPACE               "storage"
#define NVS_KEY_CONNECTED           "connected"
#define NVS_KEY_KEEP_CONNECT        "keep_connect"
#define NVS_KEY_PEER_ADDR           "peer_addr"
#define WIFI_CONNECTED_BIT          BIT0
#define WIFI_FAIL_BIT               BIT1
#define SLAVE_BROADCAST_MAC         { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF }
#define DISCONNECTED_TIMEOUT        6 * 1000 * 1000
#define ESPNOW_QUEUE_SIZE           6
#define CURRENT_INDEX               0
#define COUNT_DISCONNECTED          2
#define MAX_DATA_LEN                250
#define MAX_PAYLOAD_LEN             120 
#define PACKED_MSG_SIZE             20
=======
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
>>>>>>> 0d65c9acca272f1193113c0af4e2c3e13a3f601f
#define IS_BROADCAST_ADDR(addr)     (memcmp(addr, s_slave_broadcast_mac, ESP_NOW_ETH_ALEN) == 0)

typedef struct {
    uint8_t peer_addr[ESP_NOW_ETH_ALEN];
<<<<<<< HEAD
    bool connected;  
    int32_t count_keep_connect;                          
=======
    bool connected;                            
>>>>>>> 0d65c9acca272f1193113c0af4e2c3e13a3f601f
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

<<<<<<< HEAD
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
} sensor_data_t;

typedef struct {
    uint8_t type;                               //[1 bytes]   Broadcast or unicast ESPNOW data.
    uint16_t seq_num;                           //[2 bytes]   Sequence number of ESPNOW data.
    uint16_t crc;                               //[2 bytes]   CRC16 value of ESPNOW data.
    char message[PACKED_MSG_SIZE];              //[20 bytes]  Message espnow
    // uint8_t payload[MAX_PAYLOAD_LEN];        //[120 bytes] Real payload of ESPNOW data.
    sensor_data_t payload;
} __attribute__((packed)) espnow_data_t;

=======
>>>>>>> 0d65c9acca272f1193113c0af4e2c3e13a3f601f
/* Parameters of sending ESPNOW data. */
typedef struct {
    bool unicast;                         //Send unicast ESPNOW data.
    bool broadcast;                       //Send broadcast ESPNOW data.
    uint8_t state;                        //Indicate that if has received broadcast ESPNOW data or not.
    uint32_t magic;                       //Magic number which is used to determine which device to send unicast ESPNOW data.
    uint16_t count;                       //Total count of unicast ESPNOW data to be sent.
    uint16_t delay;                       //Delay between sending two ESPNOW data, unit: ms.
    int len;                              //Length of ESPNOW data to be sent, unit: byte.
<<<<<<< HEAD
    uint8_t buffer[MAX_DATA_LEN];         //Buffer pointing to ESPNOW data.
    uint8_t dest_mac[ESP_NOW_ETH_ALEN];   //MAC address of destination device.
} slave_espnow_send_param_t;

extern mac_master_t s_master_unicast_mac;

// Function to NVS
void log_data_from_nvs();
void erase_nvs(const char *key);
void load_from_nvs(const char *key_connected, const char *key_count, const char *key_peer_addr, mac_master_t *mac_master);
void save_to_nvs(const char *key_connected, const char *key_count, const char *key_peer_addr, bool connected, int count_keep_connect, const uint8_t *peer_addr);

// Function to read temperature internal esp
void init_temperature_sensor();
float read_internal_temperature_sensor(void);

// Function to wifi
void event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);
void slave_wifi_init(void);

// Function to slave espnow
void prepare_payload(espnow_data_t *espnow_data, float temperature_mcu, int rssi, float temperature_rdo, float do_value, float temperature_phg, float ph_value);
void parse_payload(espnow_data_t *espnow_data);
void espnow_data_prepare(slave_espnow_send_param_t *send_param, const char *message);
void espnow_data_parse(uint8_t *data, uint16_t data_len);
=======
    uint8_t buffer[MAX_DATA_LEN];                      //Buffer pointing to ESPNOW data.
    uint8_t dest_mac[ESP_NOW_ETH_ALEN];   //MAC address of destination device.
} slave_espnow_send_param_t;

typedef struct __attribute__((packed)) {
    uint8_t mac[6];
    float temperature_mcu;
    int rssi;
    float temperature_rdo;
    float do_value;
    float temperature_phg;
    float ph_value;
} sensor_data_t;

void init_temperature_sensor();
float read_internal_temperature_sensor(void);

void event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);
void slave_wifi_init(void);

>>>>>>> 0d65c9acca272f1193113c0af4e2c3e13a3f601f
void erase_peer(const uint8_t *peer_mac);
void add_peer(const uint8_t *peer_mac, bool encrypt); 
esp_err_t response_specified_mac(const uint8_t *dest_mac, const char *message, bool encrypt);
void slave_espnow_send_cb(const uint8_t *mac_addr, esp_now_send_status_t status);
void slave_espnow_recv_cb(const esp_now_recv_info_t *recv_info, const uint8_t *data, int len);
void slave_espnow_task(void *pvParameter);
esp_err_t slave_espnow_init(void);
<<<<<<< HEAD
void slave_espnow_deinit();
void slave_espnow_protocol();
void make_fake_data();
=======
void slave_espnow_deinit(slave_espnow_send_param_t *send_param);
void slave_espnow_protocol();
void prepare_data(sensor_data_t *data);
//char* sensor_data_to_json(const sensor_data_t *data_sensor)
>>>>>>> 0d65c9acca272f1193113c0af4e2c3e13a3f601f

#endif //SLAVE_ESPNOW_PROTOCOL_H