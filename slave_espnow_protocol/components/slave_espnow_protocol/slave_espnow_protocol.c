#include "slave_espnow_protocol.h"

static QueueHandle_t s_slave_espnow_queue;
static const uint8_t s_slave_broadcast_mac[ESP_NOW_ETH_ALEN] = SLAVE_BROADCAST_MAC;
mac_master_t s_master_unicast_mac;
slave_espnow_send_param_t *send_param; 

void add_peer(const uint8_t *peer_mac, bool encrypt) 
{
    esp_now_peer_info_t *peer = malloc(sizeof(esp_now_peer_info_t));
    if (peer == NULL) 
    {
        ESP_LOGE(TAG, "Malloc peer information fail");
        vSemaphoreDelete(s_slave_espnow_queue);
        esp_now_deinit();
        return;
    }

    memset(peer, 0, sizeof(esp_now_peer_info_t));
    peer->channel = CONFIG_ESPNOW_CHANNEL;
    peer->ifidx = ESPNOW_WIFI_IF;
    peer->encrypt = encrypt;
    memcpy(peer->lmk, CONFIG_ESPNOW_LMK, ESP_NOW_KEY_LEN);
    memcpy(peer->peer_addr, peer_mac, ESP_NOW_ETH_ALEN);

    if (!esp_now_is_peer_exist(peer_mac)) 
    {
        ESP_ERROR_CHECK(esp_now_add_peer(peer));
    } 
    else 
    {
        ESP_LOGI(TAG, "Peer already exists, modifying peer settings.");
        ESP_ERROR_CHECK(esp_now_mod_peer(peer));
    }
    free(peer); // Free the peer structure after use

}

/* Function to send a unicast response*/
esp_err_t response_specified_mac(const uint8_t *dest_mac, const char *message, bool encrypt)
{
    add_peer(dest_mac, encrypt);

    slave_espnow_send_param_t *send_param_agree = malloc(sizeof(slave_espnow_send_param_t));
    if (send_param_agree == NULL) 
    {
        ESP_LOGE(TAG, "Malloc send parameter fail");
        vSemaphoreDelete(s_slave_espnow_queue);
        esp_now_deinit();
        return ESP_FAIL;
    }
    memset(send_param_agree, 0, sizeof(slave_espnow_send_param_t));

    send_param_agree->len = strlen(message);
    send_param_agree->buffer = malloc(send_param_agree->len);
    if (send_param_agree->buffer == NULL) 
    {
        ESP_LOGE(TAG, "Malloc send buffer fail");
        free(send_param_agree);
        vSemaphoreDelete(s_slave_espnow_queue);
        esp_now_deinit();
        return ESP_FAIL;
    }
    memcpy(send_param_agree->dest_mac, dest_mac, ESP_NOW_ETH_ALEN);
    memcpy(send_param_agree->buffer, message, send_param_agree->len);

    // Send the unicast response
    if (esp_now_send(send_param_agree->dest_mac, send_param_agree->buffer, send_param_agree->len) != ESP_OK) 
    {
        ESP_LOGE(TAG, "Send error");
        slave_espnow_deinit(send_param_agree);
        vTaskDelete(NULL);
    }

    free(send_param_agree->buffer);
    free(send_param_agree);

    return ESP_OK;
}

void slave_espnow_send_cb(const uint8_t *mac_addr, esp_now_send_status_t status)
{
    slave_espnow_event_t evt;
    slave_espnow_event_send_cb_t *send_cb = &evt.info.send_cb;

    if (mac_addr == NULL) 
    {
        ESP_LOGE(TAG, "Send cb arg error");
        return;
    }

    evt.id = SLAVE_ESPNOW_SEND_CB;
    memcpy(send_cb->mac_addr, mac_addr, ESP_NOW_ETH_ALEN);
    send_cb->status = status;
    // if (xQueueSend(s_slave_espnow_queue, &evt, ESPNOW_MAXDELAY) != pdTRUE) {
    //     ESP_LOGW(TAG, "Send send queue fail");
    // }
    ESP_LOGI(TAG, "Send callback: MAC Address " MACSTR ", Status: %s",
        MAC2STR(mac_addr), (status == ESP_NOW_SEND_SUCCESS) ? "Success" : "Fail");
}

void slave_espnow_recv_cb(const esp_now_recv_info_t *recv_info, const uint8_t *data, int len)
{
    slave_espnow_event_t evt;
    slave_espnow_event_recv_cb_t *recv_cb = &evt.info.recv_cb;
    uint8_t * mac_addr = recv_info->src_addr;
    uint8_t * des_addr = recv_info->des_addr;

    if (mac_addr == NULL || data == NULL || len <= 0) 
    {
        ESP_LOGE(TAG, "Receive cb arg error");
        return;
    }

    evt.id = SLAVE_ESPNOW_RECV_CB;
    memcpy(recv_cb->mac_addr, mac_addr, ESP_NOW_ETH_ALEN);
    recv_cb->data = malloc(len);
    if (recv_cb->data == NULL) 
    {
        ESP_LOGE(TAG, "Malloc receive data fail");
        return;
    }
    memcpy(recv_cb->data, data, len);
    recv_cb->data_len = len;
    // if (xQueueSend(s_slave_espnow_queue, &evt, ESPNOW_MAXDELAY) != pdTRUE) {
    //     ESP_LOGW(TAG, "Send receive queue fail");
    //     free(recv_cb->data);
    // }

    // Check reception according to unicast 
    if (!IS_BROADCAST_ADDR(des_addr)) 
    {
        ESP_LOGI(TAG, "_________________________________");
        ESP_LOGI(TAG, "Receive unicast ESPNOW data");
        ESP_LOGI(TAG, "Received data from MAC: " MACSTR ", Data Length: %d, Data: %.*s",
            MAC2STR(recv_cb->mac_addr), recv_cb->data_len, recv_cb->data_len, recv_cb->data);       
        
        switch (s_master_unicast_mac.connected) 
        {
            case false:
                // Check if the received data is MASTER_AGREE_CONNECT_MSG
                if (recv_cb->data_len >= strlen(MASTER_AGREE_CONNECT_MSG) && memcmp(recv_cb->data, MASTER_AGREE_CONNECT_MSG, recv_cb->data_len) == 0) 
                {
                    memcpy(s_master_unicast_mac.peer_addr, recv_cb->mac_addr, ESP_NOW_ETH_ALEN);
                    ESP_LOGI(TAG, "Added MAC Master " MACSTR " SUCCESS", MAC2STR(s_master_unicast_mac.peer_addr));
                    ESP_LOGW(TAG, "Response to MAC " MACSTR " SAVED MAC Master", MAC2STR(s_master_unicast_mac.peer_addr));
                    s_master_unicast_mac.connected = true;
                    s_master_unicast_mac.start_time =  esp_timer_get_time();
                    response_specified_mac(s_master_unicast_mac.peer_addr, SLAVE_SAVED_MAC_MSG, false);
                    add_peer(s_master_unicast_mac.peer_addr, true);

                }
                break;

            case true:
                // Check if the received data is CHECK_CONNECTION_MSG
                if (recv_cb->data_len >= strlen(CHECK_CONNECTION_MSG) && memcmp(recv_cb->data, CHECK_CONNECTION_MSG, recv_cb->data_len) == 0) 
                {
                    s_master_unicast_mac.start_time = esp_timer_get_time();;
                    ESP_LOGW(TAG, "Response to MAC " MACSTR " CHECK CONNECT", MAC2STR(s_master_unicast_mac.peer_addr));
                    response_specified_mac(s_master_unicast_mac.peer_addr, STILL_CONNECTED_MSG, true);
                }
                break;
        }
    }         
}

void slave_espnow_task(void *pvParameter)
{
    slave_espnow_send_param_t *send_param = (slave_espnow_send_param_t *)pvParameter;
    
    send_param->len = strlen(REQUEST_CONNECTION_MSG);
    send_param->buffer = malloc(send_param->len);
    if (send_param->buffer == NULL) 
    {
        ESP_LOGE(TAG, "Malloc send buffer fail");
        free(send_param);
        vSemaphoreDelete(s_slave_espnow_queue);
        esp_now_deinit();
    }
    memcpy(send_param->dest_mac, s_slave_broadcast_mac, ESP_NOW_ETH_ALEN);
    memcpy(send_param->buffer, REQUEST_CONNECTION_MSG, send_param->len);

    while (true) 
    {
        switch (s_master_unicast_mac.connected) 
        {
            case false:
                /* Start sending broadcast ESPNOW data. */
                ESP_LOGW(TAG, "---------------------------------");
                ESP_LOGW(TAG, "Start sending broadcast data");
                if (esp_now_send(send_param->dest_mac, send_param->buffer, send_param->len) != ESP_OK) 
                {
                    ESP_LOGE(TAG, "Send error");
                    slave_espnow_deinit(send_param);
                    vTaskDelete(NULL);
                }
                break;

            case true:

                s_master_unicast_mac.end_time =  esp_timer_get_time();
                uint64_t elapsed_time = s_master_unicast_mac.end_time - s_master_unicast_mac.start_time;

                ESP_LOGI(TAG, "Elapsed time: %llu microseconds", elapsed_time);

                if (elapsed_time > 13 * 1000000)
                {
                    ESP_LOGW(TAG, "Time Out !");
                    s_master_unicast_mac.connected = false;
                }
                break;
        }
        vTaskDelay(pdMS_TO_TICKS(1000)); // Delay 1 seconds
    }
}

esp_err_t slave_espnow_init(void)
{
    s_slave_espnow_queue = xQueueCreate(ESPNOW_QUEUE_SIZE, sizeof(slave_espnow_event_t));
    if (s_slave_espnow_queue == NULL) 
    {
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
    /* Set primary slave key. */
    ESP_ERROR_CHECK( esp_now_set_pmk((uint8_t *)CONFIG_ESPNOW_PMK) );

    /* Add broadcast peer information to peer list. */
    add_peer(s_slave_broadcast_mac, false);

    /* Initialize sending parameters. */
    send_param = malloc(sizeof(slave_espnow_send_param_t));
    if (send_param == NULL) 
    {
        ESP_LOGE(TAG, "Malloc send parameter fail");
        vSemaphoreDelete(s_slave_espnow_queue);
        esp_now_deinit();
        return ESP_FAIL;
    }
    memset(send_param, 0, sizeof(slave_espnow_send_param_t));

    return ESP_OK;
}

void slave_espnow_deinit(slave_espnow_send_param_t *send_param)
{
    free(send_param->buffer);
    free(send_param);
    vSemaphoreDelete(s_slave_espnow_queue);
    esp_now_deinit();
}

void slave_espnow_protocol()
{
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) 
    {
        ESP_ERROR_CHECK( nvs_flash_erase() );
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK( ret );

    slave_wifi_init();

    slave_espnow_init();

    xTaskCreate(slave_espnow_task, "slave_espnow_task", 4096, send_param, 4, NULL);
}