#include "master_espnow_protocol.h"

static int current_index = 0;
static QueueHandle_t s_master_espnow_queue;
static const uint8_t s_master_broadcast_mac[ESP_NOW_ETH_ALEN] = MASTER_BROADCAST_MAC;
// Array to store allowed slaves
list_slaves_t allowed_connect_slaves[MAX_SLAVES];
list_slaves_t waiting_allow_connect_slaves[MAX_SLAVES];

// Function to initialize allowed slaves with hard-coded values
void get_allowed_slaves_from_nvs() {
    // Clear the array
    memset(allowed_connect_slaves, 0, sizeof(allowed_connect_slaves));
    
    // Example hard-coded MAC addresses and statuses
    uint8_t mac1[ESP_NOW_ETH_ALEN] = {0x1A, 0x2B, 0x3C, 0x4D, 0x5E, 0x6F};
    uint8_t mac2[ESP_NOW_ETH_ALEN] = {0xA4, 0xE0, 0x3A, 0xFF, 0x76, 0xC1};
    uint8_t mac3[ESP_NOW_ETH_ALEN] = {0xdc, 0xda, 0x0c, 0x0d, 0x41, 0xaa};
    
    // Add MAC addresses and statuses to the list
    memcpy(allowed_connect_slaves[0].peer_addr, mac1, ESP_NOW_ETH_ALEN);
    allowed_connect_slaves[0].status = true;   // Offline
    
    memcpy(allowed_connect_slaves[1].peer_addr, mac2, ESP_NOW_ETH_ALEN);
    allowed_connect_slaves[1].status = true;    // Online
    
    memcpy(allowed_connect_slaves[2].peer_addr, mac3, ESP_NOW_ETH_ALEN);
    allowed_connect_slaves[2].status = false;    // Online
    
    // Optionally, log the initialized values
    for (int i = 0; i < 3; i++) {
        ESP_LOGI("Init Allowed Slaves", "Slave %d: " MACSTR " Status: %s",
                 i, MAC2STR(allowed_connect_slaves[i].peer_addr), allowed_connect_slaves[i].status ? "Online" : "Offline");
    }
}

// Function to save IP MAC Slave waiting to allow
void add_to_waiting_allow_connect_slaves(const uint8_t *mac_addr) {
    for (int i = 0; i < MAX_SLAVES; i++) {
        if (memcmp(waiting_allow_connect_slaves[i].peer_addr, mac_addr, ESP_NOW_ETH_ALEN) == 0) {
            // The MAC address already exists in the list, no need to add it
            ESP_LOGI(TAG, "MAC " MACSTR " already in waiting allow connect list", MAC2STR(mac_addr));
            return;
        }
    }
    
    // Find an empty spot to add a new MAC address
    for (int i = 0; i < MAX_SLAVES; i++) {
        uint8_t zero_mac[ESP_NOW_ETH_ALEN] = {0};
        if (memcmp(waiting_allow_connect_slaves[i].peer_addr, zero_mac, ESP_NOW_ETH_ALEN) == 0) {
            // Empty location, add new MAC address here
            memcpy(waiting_allow_connect_slaves[i].peer_addr, mac_addr, ESP_NOW_ETH_ALEN);
            ESP_LOGI(TAG, "Added MAC " MACSTR " to waiting allow connect list", MAC2STR(mac_addr));
            return;
        }
    }
    
    // If the list is full, replace the MAC address at the current index in the ring list    
    memcpy(waiting_allow_connect_slaves[current_index].peer_addr, mac_addr, ESP_NOW_ETH_ALEN);
    ESP_LOGW(TAG, "Waiting allow connect list is full, replaced MAC at index %d with " MACSTR, current_index, MAC2STR(mac_addr));

    // Update index for next addition
    current_index = (current_index + 1) % MAX_SLAVES;
}


/* WiFi should start before using ESPNOW */
void master_wifi_init(void)
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

void master_espnow_send_cb(const uint8_t *mac_addr, esp_now_send_status_t status)
{
    master_espnow_event_t evt;
    master_espnow_event_send_cb_t *send_cb = &evt.info.send_cb;

    if (mac_addr == NULL) {
        ESP_LOGE(TAG, "Send cb arg error");
        return;
    }

    evt.id = MASTER_ESPNOW_SEND_CB;
    memcpy(send_cb->mac_addr, mac_addr, ESP_NOW_ETH_ALEN);
    send_cb->status = status;
    if (xQueueSend(s_master_espnow_queue, &evt, ESPNOW_MAXDELAY) != pdTRUE) {
        ESP_LOGW(TAG, "Send send queue fail");
    }

    ESP_LOGI(TAG, "Send callback: MAC Address " MACSTR ", Status: %s",
             MAC2STR(mac_addr), (status == ESP_NOW_SEND_SUCCESS) ? "Success" : "Fail");
}

/* Function to send a unicast response "Agree to connect"*/
esp_err_t response_agree_connect(const uint8_t *dest_mac, const char *message)
{
    esp_now_peer_info_t *peer_allow_connect = malloc(sizeof(esp_now_peer_info_t));
    if (peer_allow_connect == NULL) {
        ESP_LOGE(TAG, "Malloc peer information fail");
        vSemaphoreDelete(s_master_espnow_queue);
        esp_now_deinit();
        return ESP_FAIL;
    }

    memset(peer_allow_connect, 0, sizeof(esp_now_peer_info_t));
    peer_allow_connect->channel = CONFIG_ESPNOW_CHANNEL;
    peer_allow_connect->ifidx = ESPNOW_WIFI_IF;
    peer_allow_connect->encrypt = false;
    memcpy(peer_allow_connect->peer_addr, dest_mac, ESP_NOW_ETH_ALEN);

    if (!esp_now_is_peer_exist(dest_mac)) {
        ESP_ERROR_CHECK(esp_now_add_peer(peer_allow_connect));
    } else {
        ESP_LOGI(TAG, "Peer already exists, modifying peer settings.");
        ESP_ERROR_CHECK(esp_now_mod_peer(peer_allow_connect));
    }
    free(peer_allow_connect);

    master_espnow_send_param_t *send_param_agree = malloc(sizeof(master_espnow_send_param_t));
    if (send_param_agree == NULL) {
        ESP_LOGE(TAG, "Malloc send parameter fail");
        vSemaphoreDelete(s_master_espnow_queue);
        esp_now_deinit();
        return ESP_FAIL;
    }

    memset(send_param_agree, 0, sizeof(master_espnow_send_param_t));
    send_param_agree->unicast = true;
    send_param_agree->broadcast = false;
    send_param_agree->state = 0;
    send_param_agree->magic = esp_random();
    send_param_agree->count = CONFIG_ESPNOW_SEND_COUNT;
    send_param_agree->delay = CONFIG_ESPNOW_SEND_DELAY;
    send_param_agree->len = strlen(message);
    send_param_agree->buffer = malloc(send_param_agree->len);
    if (send_param_agree->buffer == NULL) {
        ESP_LOGE(TAG, "Malloc send buffer fail");
        free(send_param_agree);
        vSemaphoreDelete(s_master_espnow_queue);
        esp_now_deinit();
        return ESP_FAIL;
    }
    memcpy(send_param_agree->buffer, message, send_param_agree->len);
    memcpy(send_param_agree->dest_mac, dest_mac, ESP_NOW_ETH_ALEN);

    // Send the unicast response
    if (esp_now_send(send_param_agree->dest_mac, send_param_agree->buffer, send_param_agree->len) != ESP_OK) {
        ESP_LOGE(TAG, "Send error");
        master_espnow_deinit(send_param_agree);
        vTaskDelete(NULL);
    }

    return ESP_OK;
}

void master_espnow_recv_cb(const esp_now_recv_info_t *recv_info, const uint8_t *data, int len)
{
    master_espnow_event_t evt;
    master_espnow_event_recv_cb_t *recv_cb = &evt.info.recv_cb;
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

    evt.id = MASTER_ESPNOW_RECV_CB;
    memcpy(recv_cb->mac_addr, mac_addr, ESP_NOW_ETH_ALEN);
    recv_cb->data = malloc(len);
    if (recv_cb->data == NULL) {
        ESP_LOGE(TAG, "Malloc receive data fail");
        return;
    }
    memcpy(recv_cb->data, data, len);
    recv_cb->data_len = len;
    if (xQueueSend(s_master_espnow_queue, &evt, ESPNOW_MAXDELAY) != pdTRUE) {
        ESP_LOGW(TAG, "Send receive queue fail");
        free(recv_cb->data);
    }

    // Log the MAC address, destination address, and received data
    ESP_LOGI(TAG, "________________________________________________________________________");
    ESP_LOGI(TAG, "Received data from MAC: " MACSTR ", Data Length: %d, Data: %.*s",
             MAC2STR(recv_cb->mac_addr), recv_cb->data_len, recv_cb->data_len, recv_cb->data);       

    if (IS_BROADCAST_ADDR(des_addr)) {
        ESP_LOGI(TAG, "Receive broadcast ESPNOW data");
        
        bool found = false;
        // Check if the source MAC address is in the allowed slaves list
        for (int i = 0; i < MAX_SLAVES; i++) {
            if (memcmp(allowed_connect_slaves[i].peer_addr, recv_cb->mac_addr, ESP_NOW_ETH_ALEN) == 0) {
                if (!allowed_connect_slaves[i].status) { //Slave in status Offline
                    // Prepare and send unicast response
                    response_agree_connect(recv_cb->mac_addr, RESPONSE_AGREE_CONNECT);        
                }
                found = true;
                break;
            }
        }

        if (!found) {
            // Call a function to add the slave to the waiting_allow_connect_slaves list
            add_to_waiting_allow_connect_slaves(recv_cb->mac_addr);
        }
  
    } else {
        ESP_LOGI(TAG, "Receive unicast ESPNOW data");
        
    }
}

esp_err_t master_espnow_init(void)
{
    master_espnow_send_param_t *send_param;

    s_master_espnow_queue = xQueueCreate(ESPNOW_QUEUE_SIZE, sizeof(master_espnow_event_t));
    if (s_master_espnow_queue == NULL) {
        ESP_LOGE(TAG, "Create mutex fail");
        return ESP_FAIL;
    }

    /* Initialize ESPNOW and register sending and receiving callback function. */
    ESP_ERROR_CHECK( esp_now_init() );
    ESP_ERROR_CHECK( esp_now_register_send_cb(master_espnow_send_cb) );
    ESP_ERROR_CHECK( esp_now_register_recv_cb(master_espnow_recv_cb) );
// #if CONFIG_ESPNOW_ENABLE_POWER_SAVE
//     ESP_ERROR_CHECK( esp_now_set_wake_window(CONFIG_ESPNOW_WAKE_WINDOW) );
//     ESP_ERROR_CHECK( esp_wifi_connectionless_module_set_wake_interval(CONFIG_ESPNOW_WAKE_INTERVAL) );
// #endif
//     /* Set primary master key. */
//     ESP_ERROR_CHECK( esp_now_set_pmk((uint8_t *)CONFIG_ESPNOW_PMK) );

    /* Add broadcast peer information to peer list. */
    esp_now_peer_info_t *peer = malloc(sizeof(esp_now_peer_info_t));
    if (peer == NULL) {
        ESP_LOGE(TAG, "Malloc peer information fail");
        vSemaphoreDelete(s_master_espnow_queue);
        esp_now_deinit();
        return ESP_FAIL;
    }
    memset(peer, 0, sizeof(esp_now_peer_info_t));
    peer->channel = CONFIG_ESPNOW_CHANNEL;
    peer->ifidx = ESPNOW_WIFI_IF;
    peer->encrypt = false;
    memcpy(peer->peer_addr, s_master_broadcast_mac, ESP_NOW_ETH_ALEN);
    ESP_ERROR_CHECK( esp_now_add_peer(peer) );
    free(peer);

    /* Initialize sending parameters. */
    send_param = malloc(sizeof(master_espnow_send_param_t));
    if (send_param == NULL) {
        ESP_LOGE(TAG, "Malloc send parameter fail");
        vSemaphoreDelete(s_master_espnow_queue);
        esp_now_deinit();
        return ESP_FAIL;
    }
    memset(send_param, 0, sizeof(master_espnow_send_param_t));
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
        vSemaphoreDelete(s_master_espnow_queue);
        esp_now_deinit();
        return ESP_FAIL;
    }
    memcpy(send_param->dest_mac, s_master_broadcast_mac, ESP_NOW_ETH_ALEN);
    // master_espnow_data_prepare(send_param);

    // xTaskCreate(example_espnow_task, "example_espnow_task", 2048, send_param, 4, NULL);

    return ESP_OK;
}

void master_espnow_deinit(master_espnow_send_param_t *send_param)
{
    free(send_param->buffer);
    free(send_param);
    vSemaphoreDelete(s_master_espnow_queue);
    esp_now_deinit();
}
