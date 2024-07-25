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

static const char *TAG = "ESP-NOW MASTER";

// Global variables
static uint8_t peer_list[MAX_PEERS][ESP_NOW_ETH_ALEN];
static int peer_count = 0;
static SemaphoreHandle_t send_sem;
static int current_peer_index = 0;
static uint16_t message_index = 0;
static const uint8_t IGNORE_MAC[ESP_NOW_ETH_ALEN] = {0xec, 0xda, 0x3b, 0x54, 0xb5, 0x9c};
static char last_sent_data[128]; // Đảm bảo kích thước đủ lớn để chứa dữ liệu gửi

// Function to concatenate data with index
static void concatenate_data_with_index(const char *data, uint16_t index, char *buffer, size_t buffer_size)
{
    snprintf(buffer, buffer_size, "%s_%u", data, index);
}

// Callback function to handle received data
static void example_espnow_recv_cb(const esp_now_recv_info_t *recv_info, const uint8_t *data, int len)
{
    if (memcmp(recv_info->src_addr, IGNORE_MAC, ESP_NOW_ETH_ALEN) == 0) {
        ESP_LOGI(TAG, "Ignoring data from MAC address: " MACSTR, MAC2STR(recv_info->src_addr));
        return;
    }

    ESP_LOGI(TAG, "Receive callback: " MACSTR ", len: %d", MAC2STR(recv_info->src_addr), len);
    ESP_LOGI(TAG, "Data: %.*s", len, data);

    // Convert data to a null-terminated string
    char data_str[128];
    if (len >= sizeof(data_str)) {
        len = sizeof(data_str) - 1; // Ensure null-termination
    }
    memcpy(data_str, data, len);
    data_str[len] = '\0'; // Null-terminate the string

    // Find the index in the data
    char *underscore_pos = strchr(data_str, '_');
    if (underscore_pos != NULL) {
        // Move past the underscore to get the index part
        int index = atoi(underscore_pos + 1);
        index++;
        ESP_LOGI(TAG, "Message index: %d", message_index);
        ESP_LOGI(TAG, "Extracted index: %d", index);
        if (index != message_index)
        {
            for (int i = 0; i < 3; i++) {
                ESP_LOGI(TAG, "Resending data: %s", last_sent_data);
                esp_err_t result = esp_now_send(peer_list[current_peer_index], (const uint8_t *)last_sent_data, strlen(last_sent_data));
                if (result != ESP_OK) {
                    ESP_LOGE(TAG, "Send error to peer: " MACSTR ", error: %d", MAC2STR(peer_list[current_peer_index]), result);
                } else{
                    break;
                }
                // Wait a bit before sending the next retry
                vTaskDelay(500 / portTICK_PERIOD_MS); // 500 ms delay
            }
        }
        
    } else {
        ESP_LOGW(TAG, "No underscore found in the data");
    }


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
        
        // Add peer to ESPNOW
        esp_now_peer_info_t peer_info = {
            .peer_addr = {0},
            .channel = CONFIG_ESPNOW_CHANNEL,
            .encrypt = false,
        };
        memcpy(peer_info.peer_addr, recv_info->src_addr, ESP_NOW_ETH_ALEN);
        esp_now_add_peer(&peer_info);
    } else if (peer_count >= MAX_PEERS) {
        ESP_LOGW(TAG, "Peer list full. Cannot add new peer.");
    }

    // Release the semaphore to allow sending new data
    xSemaphoreGive(send_sem);
}

// Callback function to handle send status
static void example_espnow_send_cb(const uint8_t *mac_addr, esp_now_send_status_t status)
{
    ESP_LOGI(TAG, "Send callback: " MACSTR ", status: %d", MAC2STR(mac_addr), status);
}

// Task to send data unicast to all peers
static void example_espnow_task(void *pvParameter)
{
    char send_buffer[128]; // Adjust size as needed

    while (1) {
        if (peer_count > 0) {
            // Wait for the semaphore to be released
            if (xSemaphoreTake(send_sem, portMAX_DELAY) == pdTRUE) {
                // Concatenate data with the current index
                concatenate_data_with_index(DATA_TO_SEND, message_index, send_buffer, sizeof(send_buffer));

                // Skip sending to the ignored MAC address
                while (memcmp(peer_list[current_peer_index], IGNORE_MAC, ESP_NOW_ETH_ALEN) == 0) {
                    current_peer_index = (current_peer_index + 1) % peer_count;
                }

                ESP_LOGI(TAG, "_______________________________________________");
                ESP_LOGI(TAG, "Sending unicast data to: " MACSTR, MAC2STR(peer_list[current_peer_index]));
                esp_err_t result = esp_now_send(peer_list[current_peer_index], (const uint8_t *)send_buffer, strlen(send_buffer));
                if (result != ESP_OK) {
                    ESP_LOGE(TAG, "Send error to peer: " MACSTR ", error: %d", MAC2STR(peer_list[current_peer_index]), result);
                }

                // Save the data to retry later
                strncpy(last_sent_data, send_buffer, sizeof(last_sent_data) - 1);
                last_sent_data[sizeof(last_sent_data) - 1] = '\0'; // Ensure null-termination

                // Increment the message index for the next send
                message_index++;

                // Move to the next peer
                current_peer_index = (current_peer_index + 1) % peer_count;
            }
        }

        // Delay before checking if semaphore is available again
        vTaskDelay(1000 / portTICK_PERIOD_MS); // 1 second delay
    }
}

static esp_err_t example_espnow_init(void)
{
    // Initialize ESPNOW and register sending and receiving callback functions
    ESP_ERROR_CHECK(esp_now_init());
    ESP_ERROR_CHECK(esp_now_register_send_cb(example_espnow_send_cb));
    ESP_ERROR_CHECK(esp_now_register_recv_cb(example_espnow_recv_cb));

    // Create a semaphore for controlling data sending
    send_sem = xSemaphoreCreateBinary();
    if (send_sem == NULL) {
        ESP_LOGE(TAG, "Failed to create semaphore");
        return ESP_FAIL;
    }

    // Create a task to send data to all peers
    xTaskCreate(example_espnow_task, "example_espnow_task", 4096, NULL, 4, NULL);

    return ESP_OK;
}

// WiFi should start before using ESPNOW
static void example_wifi_init(void)
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_ERROR_CHECK(esp_wifi_set_channel(CONFIG_ESPNOW_CHANNEL, WIFI_SECOND_CHAN_NONE));

#if CONFIG_ESPNOW_ENABLE_LONG_RANGE
    ESP_ERROR_CHECK(esp_wifi_set_protocol(WIFI_IF_STA, WIFI_PROTOCOL_11B|WIFI_PROTOCOL_11G|WIFI_PROTOCOL_11N|WIFI_PROTOCOL_LR));
#endif
}

void app_main(void)
{
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    example_wifi_init();
    example_espnow_init();
}
