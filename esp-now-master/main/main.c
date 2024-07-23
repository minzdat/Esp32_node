#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "esp_event.h"
#include "esp_wifi.h"
#include "esp_now.h"

// Configuration Macros
#define ESPNOW_MAXDELAY        512
#define ESPNOW_QUEUE_SIZE      10
#define SEND_DELAY_MS          2000  // 2 seconds
#define SEND_BUFFER_LEN        100
#define SEND_TEXT              "Hello from Server!"

// MACRO for MAC address formatting
#define MAC2STR(a)             (a)[0], (a)[1], (a)[2], (a)[3], (a)[4], (a)[5]
#define MACSTR                 "%02x:%02x:%02x:%02x:%02x:%02x"

// Tag for logging
static const char *TAG = "espnow_example";

// Global variables
static QueueHandle_t s_example_espnow_queue;
static uint8_t s_example_broadcast_mac[ESP_NOW_ETH_ALEN] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };

// Structures
typedef struct {
    uint8_t dest_mac[ESP_NOW_ETH_ALEN];
    uint8_t buffer[250];
    size_t len;
    uint32_t delay;
} example_espnow_send_param_t;

typedef struct {
    uint8_t mac_addr[ESP_NOW_ETH_ALEN];
    uint8_t *data;
    int data_len;
} example_espnow_event_recv_cb_t;

typedef struct {
    int id;
    union {
        example_espnow_event_recv_cb_t recv_cb;
    } info;
} example_espnow_event_t;

typedef struct {
    uint8_t mac_addr[ESP_NOW_ETH_ALEN];
} example_peer_info_t;

static example_peer_info_t *peer_list = NULL; // Pointer to dynamic list of peers
static int peer_count = 0;
static int max_peers = 10; // Max number of peers
static int successful_send_count = 0;  // Biến đếm số lần gửi thành công
static int successful_recv_count = 0;  // Biến đếm số lần nhận dữ liệu thành công

// Function Prototypes
static void example_espnow_send_cb(const uint8_t *mac_addr, esp_now_send_status_t status);
static void example_espnow_recv_cb(const esp_now_recv_info_t *recv_info, const uint8_t *data, int len);
static void example_espnow_task(void *pvParameter);
static esp_err_t example_espnow_init(void);
static void example_wifi_init(void);

// Functions
static void example_espnow_send_cb(const uint8_t *mac_addr, esp_now_send_status_t status)
{
    ESP_LOGI(TAG, "Send callback: " MACSTR ", status: %d", MAC2STR(mac_addr), status);
    
}

static void example_espnow_recv_cb(const esp_now_recv_info_t *recv_info, const uint8_t *data, int len)
{
    ESP_LOGI(TAG, "Receive callback: " MACSTR ", len: %d", MAC2STR(recv_info->src_addr), len);
    ESP_LOGI(TAG, "Data: %.*s", len, data);

     // Tăng biến đếm khi dữ liệu được nhận thành công
    successful_recv_count++;
    ESP_LOGI(TAG, "Receive successful. Total successful receives: %d", successful_recv_count);

    // Add the received MAC address to the peer list if it's new
    for (int i = 0; i < peer_count; i++) {
        if (memcmp(peer_list[i].mac_addr, recv_info->src_addr, ESP_NOW_ETH_ALEN) == 0) {
            // Address already in the list
            return;
        }
    }
    if (peer_count < max_peers) {
        if (peer_list == NULL) {
            peer_list = malloc(max_peers * sizeof(example_peer_info_t));
            if (peer_list == NULL) {
                ESP_LOGE(TAG, "Malloc peer list fail");
                return;
            }
        }
        memcpy(peer_list[peer_count].mac_addr, recv_info->src_addr, ESP_NOW_ETH_ALEN);
        peer_count++;
        ESP_LOGI(TAG, "New peer added: " MACSTR, MAC2STR(recv_info->src_addr));
    } else {
        ESP_LOGW(TAG, "Peer list full. Cannot add new peer.");
    }
}

static void example_espnow_task(void *pvParameter)
{
    example_espnow_send_param_t *send_param = (example_espnow_send_param_t *)pvParameter;

    ESP_LOGI(TAG, "Start sending broadcast data");

    while (1) {
        // Send broadcast data
        ESP_LOGI(TAG, "_______________________________________________");
        ESP_LOGI(TAG, "Sending broadcast data: %s", send_param->buffer);
        if (esp_now_send(s_example_broadcast_mac, send_param->buffer, send_param->len) == ESP_OK) {
            successful_send_count++;  // Tăng biến đếm khi gửi thành công
            ESP_LOGI(TAG, "Send successful. Total successful sends: %d", successful_send_count);
        } else {
            ESP_LOGE(TAG, "Send error");
        }

        // Send unicast data to each peer
        // for (int i = 0; i < peer_count; i++) {
        //     ESP_LOGI(TAG, "_______________________________________________");
        //     ESP_LOGI(TAG, "Sending unicast data to: " MACSTR, MAC2STR(peer_list[i].mac_addr));
        //     if (esp_now_send(peer_list[i].mac_addr, send_param->buffer, send_param->len) != ESP_OK) {
        //         ESP_LOGE(TAG, "Send error to peer: " MACSTR, MAC2STR(peer_list[i].mac_addr));
        //     }
        // }

        if (send_param->delay > 0) {
            vTaskDelay(send_param->delay / portTICK_PERIOD_MS);
        }
    }
}

static esp_err_t example_espnow_init(void)
{
    example_espnow_send_param_t *send_param;

    s_example_espnow_queue = xQueueCreate(ESPNOW_QUEUE_SIZE, sizeof(example_espnow_event_t));
    if (s_example_espnow_queue == NULL) {
        ESP_LOGE(TAG, "Create queue fail");
        return ESP_FAIL;
    }

    esp_err_t err = esp_now_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "ESP-NOW init failed: %s", esp_err_to_name(err));
        vQueueDelete(s_example_espnow_queue);
        return err;
    }

    ESP_ERROR_CHECK(esp_now_register_send_cb(example_espnow_send_cb));
    ESP_ERROR_CHECK(esp_now_register_recv_cb(example_espnow_recv_cb));

    esp_now_peer_info_t *peer = malloc(sizeof(esp_now_peer_info_t));
    if (peer == NULL) {
        ESP_LOGE(TAG, "Malloc peer information fail");
        vQueueDelete(s_example_espnow_queue);
        esp_now_deinit();
        return ESP_FAIL;
    }

    memset(peer, 0, sizeof(esp_now_peer_info_t));
    peer->channel = 0;
    peer->ifidx = ESP_IF_WIFI_STA;
    peer->encrypt = false;
    memcpy(peer->peer_addr, s_example_broadcast_mac, ESP_NOW_ETH_ALEN);
    ESP_ERROR_CHECK(esp_now_add_peer(peer));
    free(peer);

    send_param = malloc(sizeof(example_espnow_send_param_t));
    if (send_param == NULL) {
        ESP_LOGE(TAG, "Malloc send parameter fail");
        vQueueDelete(s_example_espnow_queue);
        esp_now_deinit();
        return ESP_FAIL;
    }

    memset(send_param, 0, sizeof(example_espnow_send_param_t));
    send_param->delay = SEND_DELAY_MS;
    send_param->len = SEND_BUFFER_LEN;
    snprintf((char *)send_param->buffer, sizeof(send_param->buffer), SEND_TEXT);

    xTaskCreate(example_espnow_task, "example_espnow_task", 2048, send_param, 4, NULL);

    return ESP_OK;
}

static void example_wifi_init(void)
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_ERROR_CHECK(esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE));
}

void app_main(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    example_wifi_init();
    ESP_ERROR_CHECK(example_espnow_init());
}
