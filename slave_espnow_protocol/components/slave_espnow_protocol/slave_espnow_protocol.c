#include "slave_espnow_protocol.h"

static const uint8_t s_slave_broadcast_mac[ESP_NOW_ETH_ALEN] = SLAVE_BROADCAST_MAC;
static uint8_t s_master_unicast_mac[ESP_NOW_ETH_ALEN];
static QueueHandle_t s_slave_espnow_queue;
static TaskHandle_t slave_espnow_task_handle = NULL;

esp_err_t add_peer(const uint8_t *peer_mac) {
    /* Add broadcast peer information to peer list. */
    esp_now_peer_info_t *peer = malloc(sizeof(esp_now_peer_info_t));
    if (peer == NULL) {
        ESP_LOGE(TAG, "Malloc peer information fail");
        vSemaphoreDelete(s_slave_espnow_queue);
        esp_now_deinit();
        return ESP_FAIL;
    }
    memset(peer, 0, sizeof(esp_now_peer_info_t));
    peer->channel = CONFIG_ESPNOW_CHANNEL;
    peer->ifidx = ESPNOW_WIFI_IF;
    peer->encrypt = false;
    memcpy(peer->peer_addr, peer_mac, ESP_NOW_ETH_ALEN);
    
    esp_err_t result = esp_now_add_peer(peer);
    free(peer);

    if (result != ESP_OK) {
        ESP_LOGE(TAG, "Add peer fail: %s", esp_err_to_name(result));
    }
    return result;
}

void slave_espnow_send_cb(const uint8_t *mac_addr, esp_now_send_status_t status)
{
    slave_espnow_event_t evt;
    slave_espnow_event_send_cb_t *send_cb = &evt.info.send_cb;

    if (mac_addr == NULL) {
        ESP_LOGE(TAG, "Send cb arg error");
        return;
    }

    evt.id = SLAVE_ESPNOW_SEND_CB;
    memcpy(send_cb->mac_addr, mac_addr, ESP_NOW_ETH_ALEN);
    send_cb->status = status;
    // if (xQueueSend(s_slave_espnow_queue, &evt, ESPNOW_MAXDELAY) != pdTRUE) {
    //     ESP_LOGW(TAG, "Send send queue fail");
    // }
    ESP_LOGI(TAG, "---------------------------------");
    ESP_LOGI(TAG, "Send callback: MAC Address " MACSTR ", Status: %s",
             MAC2STR(mac_addr), (status == ESP_NOW_SEND_SUCCESS) ? "Success" : "Fail");
}

/* Function to send a unicast response "SLAVE_SAVED_MAC_MSG"*/
esp_err_t response_saved_mac_master(const uint8_t *dest_mac, const char *message)
{
    /* Add broadcast peer information to peer list. */
    add_peer(dest_mac);

    esp_err_t result;

    if (dest_mac == NULL || message == NULL) {
        ESP_LOGE(TAG, "Response arg error");
        return ESP_ERR_INVALID_ARG;
    }

    // Log the MAC address and message to be sent
    ESP_LOGI(TAG, "Sending unicast message to MAC: " MACSTR ", Message: %s",
             MAC2STR(dest_mac), message);

    // Send the unicast message
    result = esp_now_send(dest_mac, (const uint8_t *)message, strlen(message));
    if (result != ESP_OK) {
        ESP_LOGE(TAG, "Send unicast error: %s", esp_err_to_name(result));
    } else {
        ESP_LOGI(TAG, "Unicast message sent successfully");
    }

    return result;
}

void slave_espnow_recv_cb(const esp_now_recv_info_t *recv_info, const uint8_t *data, int len)
{
    slave_espnow_event_t evt;
    slave_espnow_event_recv_cb_t *recv_cb = &evt.info.recv_cb;
    uint8_t * mac_addr = recv_info->src_addr;
    uint8_t * des_addr = recv_info->des_addr;

    if (mac_addr == NULL || data == NULL || len <= 0) {
        ESP_LOGE(TAG, "Receive cb arg error");
        return;
    }

    // if (IS_BROADCAST_ADDR(des_addr)) {
    //     /* If added a peer with encryption before, the receive packets may be
    //      * encrypted as peer-to-peer message or unencrypted over the broadcast channel.
    //      * Users can check the destination address to distinguish it.
    //      */
    //     ESP_LOGD(TAG, "Receive broadcast ESPNOW data");
    // } else {
    //     ESP_LOGD(TAG, "Receive unicast ESPNOW data");
    // }

    evt.id = SLAVE_ESPNOW_RECV_CB;
    memcpy(recv_cb->mac_addr, mac_addr, ESP_NOW_ETH_ALEN);
    recv_cb->data = malloc(len);
    if (recv_cb->data == NULL) {
        ESP_LOGE(TAG, "Malloc receive data fail");
        return;
    }
    memcpy(recv_cb->data, data, len);
    recv_cb->data_len = len;
    // if (xQueueSend(s_slave_espnow_queue, &evt, ESPNOW_MAXDELAY) != pdTRUE) {
    //     ESP_LOGW(TAG, "Send receive queue fail");
    //     free(recv_cb->data);
    // }

    // Log the MAC address, destination address, and received data
    ESP_LOGI(TAG, "_________________________________");
    ESP_LOGI(TAG, "Received data from MAC: " MACSTR ", Data Length: %d, Data: %.*s",
             MAC2STR(recv_cb->mac_addr), recv_cb->data_len, recv_cb->data_len, recv_cb->data);       

    // Check reception according to unicast 
    if (!IS_BROADCAST_ADDR(des_addr)) {
        ESP_LOGW(TAG, "Receive unicast ESPNOW data");
        
        // Check if the received data is MASTER_AGREE_CONNECT_MSG
        if (recv_cb->data_len >= strlen(MASTER_AGREE_CONNECT_MSG) && memcmp(recv_cb->data, MASTER_AGREE_CONNECT_MSG, recv_cb->data_len) == 0) {
            memcpy(s_master_unicast_mac, recv_cb->mac_addr, ESP_NOW_ETH_ALEN);
            ESP_LOGI(TAG, "Added MAC Master " MACSTR " SUCCESS", MAC2STR(s_master_unicast_mac));

            // Send response message to s_master_unicast_mac
            esp_err_t result = response_saved_mac_master(s_master_unicast_mac, SLAVE_SAVED_MAC_MSG);
            if (result != ESP_OK) {
                ESP_LOGE(TAG, "Failed to send response to MAC Master");
            }
            else {
                // Delete task slave_espnow_task
                if (slave_espnow_task_handle != NULL) {
                    vTaskDelete(slave_espnow_task_handle);
                    slave_espnow_task_handle = NULL;
                    ESP_LOGI(TAG, "Task slave_espnow_task deleted.");
                }
            }
        }
    }         
}

/* WiFi should start before using ESPNOW */
void slave_wifi_init(void)
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

void slave_espnow_task(void *pvParameter)
{
    slave_espnow_send_param_t *send_param = (slave_espnow_send_param_t *)pvParameter;
    
    send_param->buffer = (uint8_t *)REQUEST_CONNECTION_MSG;
    send_param->len = strlen(REQUEST_CONNECTION_MSG);

    while (true) {
        ESP_LOGI(TAG, "Start sending broadcast data");

        /* Start sending broadcast ESPNOW data. */
        if (esp_now_send(send_param->dest_mac, send_param->buffer, send_param->len) != ESP_OK) {
            ESP_LOGE(TAG, "Send error");
            slave_espnow_deinit(send_param);
            vTaskDelete(NULL);
        }
        vTaskDelay(pdMS_TO_TICKS(2000)); // Delay 2 seconds
    }
}

esp_err_t slave_espnow_init(void)
{
    slave_espnow_send_param_t *send_param;

    s_slave_espnow_queue = xQueueCreate(ESPNOW_QUEUE_SIZE, sizeof(slave_espnow_event_t));
    if (s_slave_espnow_queue == NULL) {
        ESP_LOGE(TAG, "Create mutex fail");
        return ESP_FAIL;
    }

    /* Initialize ESPNOW and register sending and receiving callback function. */
    ESP_ERROR_CHECK( esp_now_init() );
    ESP_ERROR_CHECK( esp_now_register_send_cb(slave_espnow_send_cb) );
    ESP_ERROR_CHECK( esp_now_register_recv_cb(slave_espnow_recv_cb) );
// #if CONFIG_ESPNOW_ENABLE_POWER_SAVE
//     ESP_ERROR_CHECK( esp_now_set_wake_window(CONFIG_ESPNOW_WAKE_WINDOW) );
//     ESP_ERROR_CHECK( esp_wifi_connectionless_module_set_wake_interval(CONFIG_ESPNOW_WAKE_INTERVAL) );
// #endif
//     /* Set primary slave key. */
//     ESP_ERROR_CHECK( esp_now_set_pmk((uint8_t *)CONFIG_ESPNOW_PMK) );

    /* Add broadcast peer information to peer list. */
    add_peer(s_slave_broadcast_mac);

    /* Initialize sending parameters. */
    send_param = malloc(sizeof(slave_espnow_send_param_t));
    if (send_param == NULL) {
        ESP_LOGE(TAG, "Malloc send parameter fail");
        vSemaphoreDelete(s_slave_espnow_queue);
        esp_now_deinit();
        return ESP_FAIL;
    }
    memset(send_param, 0, sizeof(slave_espnow_send_param_t));
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
        vSemaphoreDelete(s_slave_espnow_queue);
        esp_now_deinit();
        return ESP_FAIL;
    }
    memcpy(send_param->dest_mac, s_slave_broadcast_mac, ESP_NOW_ETH_ALEN);
    // slave_espnow_data_prepare(send_param);

    xTaskCreate(slave_espnow_task, "slave_espnow_task", 4096, send_param, 4, &slave_espnow_task_handle);

    return ESP_OK;
}

void slave_espnow_deinit(slave_espnow_send_param_t *send_param)
{
    free(send_param->buffer);
    free(send_param);
    vSemaphoreDelete(s_slave_espnow_queue);
    esp_now_deinit();
}
