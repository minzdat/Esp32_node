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
#include "espnow_example.h"
#include "esp_timer.h"

// Tag for logging
static const char *TAG = "ESP-NOW SLAVE";

// Global variables
static TaskHandle_t xTaskHandle = NULL;
static QueueHandle_t s_example_espnow_queue;
static uint8_t s_example_broadcast_mac[ESP_NOW_ETH_ALEN] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
static uint8_t IGNORE_MAC[ESP_NOW_ETH_ALEN] = {0xec, 0xda, 0x3b, 0x54, 0xb5, 0x9c};
static uint8_t peer_list[MAX_PEERS][ESP_NOW_ETH_ALEN];
static int peer_count = 0;
static int successful_send_count = 0; 
static int successful_recv_count = 0;

int64_t start = 0;
int64_t stop = 0;
int64_t time_s = 0;

// Callback function to handle send status
static void example_espnow_send_cb(const uint8_t *mac_addr, esp_now_send_status_t status)
{
    start=esp_timer_get_time();
    printf("Send Status: %s\n", status == ESP_NOW_SEND_SUCCESS ? "Success" : "Fail");
}

// Callback function to handle received data
static void example_espnow_recv_cb(const esp_now_recv_info_t *recv_info, const uint8_t *data, int len)
{
    stop = esp_timer_get_time();
    time_s = stop - start;
    ESP_LOGW(TAG, "Transmission Time: %lld us", time_s);  

    int8_t rssi = recv_info->rx_ctrl->rssi;
    ESP_LOGW(TAG, "RSSI: %d", rssi);
    
    // if (memcmp(recv_info->src_addr, IGNORE_MAC, ESP_NOW_ETH_ALEN) == 0) {
    //     ESP_LOGI(TAG, "Message from ignored MAC address: " MACSTR, MAC2STR(recv_info->src_addr));
    //     return;
    // }

    ESP_LOGI(TAG, "Receive callback: " MACSTR ", len: %d", MAC2STR(recv_info->src_addr), len);
    ESP_LOGI(TAG, "Data: %.*s", len, data);

    // Save sender MAC address if it's not already in the list
    bool is_new_peer = true;
    for (int i = 0; i < peer_count; i++) {
        if (memcmp(peer_list[i], recv_info->src_addr, ESP_NOW_ETH_ALEN) == 0) {
            is_new_peer = false;
            break;
        }
    }

    if (is_new_peer && peer_count < MAX_PEERS) {
        memcpy(peer_list[peer_count], recv_info->src_addr, ESP_NOW_ETH_ALEN);
        peer_count++;
        ESP_LOGI(TAG, "New peer added: " MACSTR, MAC2STR(recv_info->src_addr));
        
        /* Add broadcast peer information to peer list. */
        esp_now_peer_info_t *peer_info = malloc(sizeof(esp_now_peer_info_t));
        if (peer_info == NULL) {
            ESP_LOGE(TAG, "Malloc peer information fail");
            // Clean up or handle the error appropriately
            return;
        }
        memset(peer_info, 0, sizeof(esp_now_peer_info_t));
        peer_info->channel = CONFIG_ESPNOW_CHANNEL;
        peer_info->ifidx = ESPNOW_WIFI_IF;
        peer_info->encrypt = true;
        memcpy(peer_info->lmk, CONFIG_ESPNOW_LMK, ESP_NOW_KEY_LEN);
        memcpy(peer_info->peer_addr, recv_info->src_addr, ESP_NOW_ETH_ALEN);
        ESP_ERROR_CHECK( esp_now_add_peer(peer_info) );
        free(peer_info);

        // Xóa task example_espnow_task
        if (xTaskHandle != NULL) {
            vTaskDelete(xTaskHandle);
            xTaskHandle = NULL;
            ESP_LOGI(TAG, "Task example_espnow_task deleted.");
        }

    } else if (peer_count >= MAX_PEERS) {
        ESP_LOGW(TAG, "Peer list full. Cannot add new peer.");
    }

    // Delay 1 seconds before sending unicast data
    vTaskDelay(1000 / portTICK_PERIOD_MS); // 1 second delay
    if (len >= strlen("Get data from Master:") && strstr((const char *)data, "Get data from Master") != NULL) {
        // Increment the counter variable when data is successfully received
        successful_recv_count++;
        ESP_LOGI(TAG, "Receive successful. Total successful receives: %d", successful_recv_count);
        for (int i = 0; i < peer_count; i++) {
            
            // if (memcmp(peer_list[i], IGNORE_MAC, ESP_NOW_ETH_ALEN) == 0) {
            //     ESP_LOGI(TAG, "Skipping ignored MAC address: " MACSTR, MAC2STR(peer_list[i]));
            //     continue;
            // }

            ESP_LOGI(TAG, "_______________________________________________");
            ESP_LOGI(TAG, "Sending unicast data to: " MACSTR, MAC2STR(peer_list[i]));
            esp_err_t result = esp_now_send(peer_list[i], data, len);
            if (result != ESP_OK) {
                ESP_LOGE(TAG, "Send error to peer: " MACSTR ", error: %d", MAC2STR(peer_list[i]), result);
            }
            else
            {
                // Increment the counter variable when sent successfully
                successful_send_count++; 
                ESP_LOGI(TAG, "Send successful. Total successful sends: %d", successful_send_count);
            }
        }
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
        esp_now_send(send_param->dest_mac, send_param->buffer, send_param->len);

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
        ESP_LOGE(TAG, "Create mutex fail");
        return ESP_FAIL;
    }

    /* Initialize ESPNOW and register sending and receiving callback function. */
    ESP_ERROR_CHECK( esp_now_init() );
    ESP_ERROR_CHECK( esp_now_register_send_cb(example_espnow_send_cb) );
    ESP_ERROR_CHECK( esp_now_register_recv_cb(example_espnow_recv_cb) );

    /* Set primary master key. */
    ESP_ERROR_CHECK( esp_now_set_pmk((uint8_t *)CONFIG_ESPNOW_PMK) );
    
    /* Add broadcast peer information to peer list. */
    esp_now_peer_info_t *peer = malloc(sizeof(esp_now_peer_info_t));
    if (peer == NULL) {
        ESP_LOGE(TAG, "Malloc peer information fail");
        vSemaphoreDelete(s_example_espnow_queue);
        esp_now_deinit();
        return ESP_FAIL;
    }
    memset(peer, 0, sizeof(esp_now_peer_info_t));
    peer->channel = CONFIG_ESPNOW_CHANNEL;
    peer->ifidx = ESPNOW_WIFI_IF;
    peer->encrypt = false;
    memcpy(peer->peer_addr, s_example_broadcast_mac, ESP_NOW_ETH_ALEN);
    ESP_ERROR_CHECK( esp_now_add_peer(peer) );
    free(peer);

    /* Initialize sending parameters. */
    send_param = malloc(sizeof(example_espnow_send_param_t));
    if (send_param == NULL) {
        ESP_LOGE(TAG, "Malloc send parameter fail");
        vSemaphoreDelete(s_example_espnow_queue);
        esp_now_deinit();
        return ESP_FAIL;
    }
    memset(send_param, 0, sizeof(example_espnow_send_param_t));
    send_param->unicast = false;
    send_param->broadcast = true;
    send_param->state = 0;
    send_param->magic = esp_random();
    send_param->count = CONFIG_ESPNOW_SEND_COUNT;
    send_param->delay = CONFIG_ESPNOW_SEND_DELAY;
    send_param->len = strlen(DATA_TO_SEND); // Độ dài của dữ liệu gửi
    send_param->buffer = malloc(send_param->len + 1); // Cấp phát thêm 1 byte cho dấu kết thúc chuỗi
    if (send_param->buffer == NULL) {
        ESP_LOGE(TAG, "Malloc send buffer fail");
        free(send_param);
        vSemaphoreDelete(s_example_espnow_queue);
        esp_now_deinit();
        return ESP_FAIL;
    }
    memcpy(send_param->buffer, DATA_TO_SEND, send_param->len);
    send_param->buffer[send_param->len] = '\0';
    memcpy(send_param->dest_mac, s_example_broadcast_mac, ESP_NOW_ETH_ALEN);

    xTaskCreate(example_espnow_task, "example_espnow_task", 2048, send_param, 4, &xTaskHandle);

    return ESP_OK;
}

/* WiFi should start before using ESPNOW */
static void example_wifi_init(void)
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK( esp_wifi_init(&cfg) );
    ESP_ERROR_CHECK( esp_wifi_set_storage(WIFI_STORAGE_RAM) );
    ESP_ERROR_CHECK( esp_wifi_set_mode(ESPNOW_WIFI_MODE) );
    ESP_ERROR_CHECK( esp_wifi_start());
    ESP_ERROR_CHECK( esp_wifi_set_channel(CONFIG_ESPNOW_CHANNEL, WIFI_SECOND_CHAN_NONE));

#if CONFIG_ESPNOW_ENABLE_LONG_RANGE
    ESP_ERROR_CHECK( esp_wifi_set_protocol(ESPNOW_WIFI_IF, WIFI_PROTOCOL_11B|WIFI_PROTOCOL_11G|WIFI_PROTOCOL_11N|WIFI_PROTOCOL_LR) );
#endif
}

void set_wifi_max_tx_power(void) {
    esp_err_t err = esp_wifi_set_max_tx_power(12);
    ESP_LOGI(TAG, "Esp set power: %s", esp_err_to_name(err));
    int8_t tx_power;
    esp_wifi_get_max_tx_power(&tx_power);
    tx_power=tx_power/4;
    ESP_LOGI(TAG, "Current max TX power: %d dBm", tx_power);
}

void app_main(void)
{
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK( nvs_flash_erase() );
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK( ret );

    example_wifi_init();

    set_wifi_max_tx_power();

    example_espnow_init();
}
