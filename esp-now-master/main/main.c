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
#include "driver/temp_sensor.h" // Include the temperature sensor driver

// Tag for logging
static const char *TAG = "ESP-NOW MASTER";

// Global variables
static SemaphoreHandle_t send_sem;
static QueueHandle_t s_example_espnow_queue;
static uint8_t peer_list[MAX_PEERS][ESP_NOW_ETH_ALEN];
static int peer_count = 0;
static int current_peer_index = 0;
static uint16_t message_index = 0;
static uint16_t send_index = 0;
static int successful_send_count = 0; 
static int successful_recv_count = 0;
static const char *RESPONSE_MSG = "Hello from Master";
static uint8_t s_example_broadcast_mac[ESP_NOW_ETH_ALEN] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
static uint8_t IGNORE_MAC[ESP_NOW_ETH_ALEN] = {0xec, 0xda, 0x3b, 0x54, 0xb5, 0x9c};
static char last_sent_data[128]; // Đảm bảo kích thước đủ lớn để chứa dữ liệu gửi
static char send_buffer[128]; // Đảm bảo kích thước đủ lớn để chứa dữ liệu gửi

int64_t start = 0;
int64_t stop = 0;
int64_t time_s = 0;


// Function to concatenate data with index
static void concatenate_data_with_index(const char *data, uint16_t index, char *buffer, size_t buffer_size)
{
    snprintf(buffer, buffer_size, "%s_%u", data, index);
}

// Function to decode index from received data
static int decode_index(const uint8_t *data, int len)
{
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
        return atoi(underscore_pos + 1);
    } else {
        ESP_LOGW(TAG, "No underscore found in the data");
        return -1; // Return -1 if no underscore is found
    }
}

// Function to send data with retries
static void send_with_retries(const uint8_t *peer_addr, const char *data, int max_retries)
{
    for (int i = 0; i < max_retries; i++) {
        ESP_LOGI(TAG, "Resending data: %s", data);
        esp_err_t result = esp_now_send(peer_addr, (const uint8_t *)data, strlen(data));
        if (result == ESP_OK) {
            break;
        } else {
            ESP_LOGE(TAG, "Send error to peer: " MACSTR ", error: %d", MAC2STR(peer_addr), result);
        }
        // Wait a bit before sending the next retry
        vTaskDelay(500 / portTICK_PERIOD_MS); // 500 ms delay
    }
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
    //     ESP_LOGI(TAG, "Ignoring data from MAC address: " MACSTR, MAC2STR(recv_info->src_addr));
    //     return;
    // }

    ESP_LOGI(TAG, "Receive callback: " MACSTR ", len: %d", MAC2STR(recv_info->src_addr), len);
    ESP_LOGI(TAG, "Data: %.*s", len, data);

    if (len >= strlen("Get data from Master:") && strstr((const char *)data, "Get data from Master") != NULL) {
        // Increment the counter variable when data is successfully received
        successful_recv_count++;
        ESP_LOGI(TAG, "Receive successful. Total successful receives: %d", successful_recv_count);
    }

    // int received_index = decode_index(data, len);
    // if (received_index != -1) {
    //     ESP_LOGI(TAG, "Send index: %d", send_index);
    //     ESP_LOGI(TAG, "Extracted index: %d", received_index);
    //     if (received_index != send_index) {
    //         // send_with_retries(peer_list[current_peer_index], last_sent_data, 3);
    //     }
    // }

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
        peer_info->encrypt = false;
        memcpy(peer_info->peer_addr, recv_info->src_addr, ESP_NOW_ETH_ALEN);
        // Add the peer before modifying it
        esp_err_t add_status = esp_now_add_peer(peer_info);
        if (add_status != ESP_OK) {
            ESP_LOGE(TAG, "Failed to add peer: " MACSTR ", error: %d", MAC2STR(recv_info->src_addr), add_status);
            free(peer_info);
            return;
        }

        // Send immediate response
        ESP_LOGI(TAG, "Sending response to new peer: " MACSTR, MAC2STR(recv_info->src_addr));
        esp_err_t result = esp_now_send(recv_info->src_addr, (const uint8_t *)RESPONSE_MSG, strlen(RESPONSE_MSG));
        if (result != ESP_OK) {
            ESP_LOGE(TAG, "Send response error to peer: " MACSTR ", error: %d", MAC2STR(recv_info->src_addr), result);
        }

        // Enable encryption for unicast communication
        memcpy(peer_info->lmk, CONFIG_ESPNOW_LMK, ESP_NOW_KEY_LEN);
        peer_info->encrypt = true;
        esp_err_t mod_status = esp_now_mod_peer(peer_info);
        if (mod_status != ESP_OK) {
            ESP_LOGE(TAG, "Failed to modify peer: " MACSTR ", error: %d", MAC2STR(recv_info->src_addr), mod_status);
        }
        free(peer_info);

    } else if (peer_count >= MAX_PEERS) {
        ESP_LOGW(TAG, "Peer list full. Cannot add new peer.");
    }

    // Release the semaphore to allow sending new data
    xSemaphoreGive(send_sem);
}

// Callback function to handle send status
static void example_espnow_send_cb(const uint8_t *mac_addr, esp_now_send_status_t status)
{
    start=esp_timer_get_time();
    printf("Send Status: %s\n", status == ESP_NOW_SEND_SUCCESS ? "Success" : "Fail");

    if (status != ESP_NOW_SEND_SUCCESS) {
        xSemaphoreGive(send_sem);
    }
}

// Task to send data unicast to all peers
static void example_espnow_task(void *pvParameter)
{
    while (1) {
        if (peer_count > 0) {
            // Wait for the semaphore to be released
            if (xSemaphoreTake(send_sem, portMAX_DELAY) == pdTRUE) {

                // Skip sending to the ignored MAC address
                // while (memcmp(peer_list[current_peer_index], IGNORE_MAC, ESP_NOW_ETH_ALEN) == 0) {
                //     current_peer_index = (current_peer_index + 1) % peer_count;
                // }

                // Concatenate data with the current index
                concatenate_data_with_index(DATA_TO_SEND, message_index, send_buffer, sizeof(send_buffer));

                ESP_LOGI(TAG, "_______________________________________________");
                ESP_LOGI(TAG, "Sending unicast data to: " MACSTR, MAC2STR(peer_list[current_peer_index]));
                esp_err_t result = esp_now_send(peer_list[current_peer_index], (const uint8_t *)send_buffer, strlen(send_buffer));
                if (result != ESP_OK) {
                    ESP_LOGE(TAG, "Send error to peer: " MACSTR ", error: %d", MAC2STR(peer_list[current_peer_index]), result);
                } 
                else{
                    // Increment the counter variable when sent successfully
                    successful_send_count++; 
                    ESP_LOGI(TAG, "Send successful. Total successful sends: %d", successful_send_count);
                }

                // Save the data to retry later
                // strncpy(last_sent_data, send_buffer, sizeof(last_sent_data) - 1);
                // last_sent_data[sizeof(last_sent_data) - 1] = '\0'; // Ensure null-termination
                // send_index = message_index;
                
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

// Initialize the temperature sensor
static void init_temp_sensor()
{
    temp_sensor_config_t temp_sensor = TSENS_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(temp_sensor_set_config(temp_sensor));
    ESP_ERROR_CHECK(temp_sensor_start());
}

// Function to read temperature sensor value
static float read_temp_sensor()
{
    float tsens_out;
    ESP_ERROR_CHECK(temp_sensor_read_celsius(&tsens_out));
    return tsens_out;
}

// Task to log temperature values
static void log_temp_task(void *pvParameter)
{
    while (1) {
        float temperature = read_temp_sensor();
        ESP_LOGE(TAG, "Temperature: %.2f C", temperature);
        vTaskDelay(1000 / portTICK_PERIOD_MS); // 1 second delay
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
    send_param->len = CONFIG_ESPNOW_SEND_LEN;
    send_param->buffer = malloc(CONFIG_ESPNOW_SEND_LEN);
    if (send_param->buffer == NULL) {
        ESP_LOGE(TAG, "Malloc send buffer fail");
        free(send_param);
        vSemaphoreDelete(s_example_espnow_queue);
        esp_now_deinit();
        return ESP_FAIL;
    }
    memcpy(send_param->dest_mac, s_example_broadcast_mac, ESP_NOW_ETH_ALEN);

    // Create a semaphore for controlling data sending
    send_sem = xSemaphoreCreateBinary();
    if (send_sem == NULL) {
        ESP_LOGE(TAG, "Failed to create semaphore");
        return ESP_FAIL;
    }

    // Create a task to send data to all peers
    xTaskCreate(example_espnow_task, "example_espnow_task", 4096, NULL, 4, NULL);

    xTaskCreate(log_temp_task, "log_temp_task", 2048, NULL, 4, NULL);

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
    init_temp_sensor(); // Initialize the temperature sensor

    set_wifi_max_tx_power();

    example_espnow_init();
}
