#include "slave_espnow_protocol.h"

static QueueHandle_t s_slave_espnow_queue;
static const uint8_t s_slave_broadcast_mac[ESP_NOW_ETH_ALEN] = SLAVE_BROADCAST_MAC;
static slave_espnow_send_param_t send_param;
mac_master_t s_master_unicast_mac;

// Hàm xóa peer của thiết bị khác
void erase_peer(const uint8_t *peer_mac) 
{
    if (esp_now_is_peer_exist(peer_mac)) 
    {
        esp_err_t del_err = esp_now_del_peer(peer_mac);
        if (del_err != ESP_OK) 
        {
            ESP_LOGE(TAG, "Failed to delete peer " MACSTR ": %s",
                     MAC2STR(peer_mac), esp_err_to_name(del_err));
        } 
        else 
        {
            ESP_LOGI(TAG, "Deleted peer " MACSTR,
                     MAC2STR(peer_mac));
        }
    }
}

void add_peer(const uint8_t *peer_mac, bool encrypt) 
{   
    esp_now_peer_info_t peer; 

    memset(&peer, 0, sizeof(esp_now_peer_info_t));
    peer.channel = CONFIG_ESPNOW_CHANNEL;
    peer.ifidx = ESPNOW_WIFI_IF;
    peer.encrypt = encrypt;
    memcpy(peer.lmk, CONFIG_ESPNOW_LMK, ESP_NOW_KEY_LEN);
    memcpy(peer.peer_addr, peer_mac, ESP_NOW_ETH_ALEN);

    if (!esp_now_is_peer_exist(peer_mac)) 
    {
        ESP_ERROR_CHECK(esp_now_add_peer(&peer));
    } 
    else 
    {
        ESP_LOGI(TAG, "Peer already exists, modifying peer settings.");
        ESP_ERROR_CHECK(esp_now_mod_peer(&peer));
    }
}

/* Function to send a unicast response*/
esp_err_t response_specified_mac(const uint8_t *dest_mac, const char *message, bool encrypt)
{
    add_peer(dest_mac, encrypt);

    slave_espnow_send_param_t send_param_agree;
    memset(&send_param_agree, 0, sizeof(slave_espnow_send_param_t));

    send_param_agree.len = strlen(message);
    
    if (send_param_agree.len > sizeof(send_param_agree.buffer)) {
        ESP_LOGE(TAG, "Message too long to fit in the buffer");
        vSemaphoreDelete(s_slave_espnow_queue);
        esp_now_deinit();
        return ESP_FAIL;
    }

    memcpy(send_param_agree.dest_mac, dest_mac, ESP_NOW_ETH_ALEN);
    memcpy(send_param_agree.buffer, message, send_param_agree.len);

    // Send the unicast response
    if (esp_now_send(send_param_agree.dest_mac, send_param_agree.buffer, send_param_agree.len) != ESP_OK) 
    {
        ESP_LOGE(TAG, "Send error");
        slave_espnow_deinit(&send_param_agree);
        vTaskDelete(NULL);
    }

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

    if (len > MAX_DATA_LEN)
    {
        ESP_LOGE(TAG, "Received data length exceeds the maximum allowed");
        return;
    }

    recv_cb->data_len = len;
    memcpy(recv_cb->data, data, recv_cb->data_len);

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
                    float temperature = read_internal_temperature_sensor();

                    char response_message[250];
                    int message_len = snprintf(response_message, sizeof(response_message), "%s: %.2f C", STILL_CONNECTED_MSG, temperature);

                    if (message_len >= sizeof(response_message)) {
                        ESP_LOGE(TAG, "Response message is too long");
                        return;
                    }
                    char *response_json =json_data();
                    s_master_unicast_mac.start_time = esp_timer_get_time();;
                    sensor_data_t data_sensor;
                    prepare_data(&data_sensor);
                    data_sensor.rssi=recv_info->rx_ctrl->rssi;

                    ESP_LOGW(TAG, "Response to MAC " MACSTR " CHECK CONNECT with temperature %.2f C", MAC2STR(s_master_unicast_mac.peer_addr), temperature);
                    // response_specified_mac(s_master_unicast_mac.peer_addr, (const char*)&data, true);
                    // response_specified_mac(s_master_unicast_mac.peer_addr, data_senosr, true);
                    esp_now_send(s_master_unicast_mac.peer_addr,(uint8_t *)&data_sensor,sizeof(data_sensor));
                    // response_specified_mac(s_master_unicast_mac.peer_addr, response_message, true);
                }
                break;
        }
    }         
}

void slave_espnow_task(void *pvParameter)
{
    slave_espnow_send_param_t *send_param = (slave_espnow_send_param_t *)pvParameter;
    
    send_param->len = strlen(REQUEST_CONNECTION_MSG);

    if (send_param->len > MAX_DATA_LEN) 
    {
        ESP_LOGE(TAG, "Message length exceeds buffer size");
        free(send_param);
        vSemaphoreDelete(s_slave_espnow_queue);
        esp_now_deinit();
        return;
    }

    memcpy(send_param->dest_mac, s_slave_broadcast_mac, ESP_NOW_ETH_ALEN);
    memcpy(send_param->buffer, REQUEST_CONNECTION_MSG, send_param->len);

    while (true) 
    {
        switch (s_master_unicast_mac.connected) 
        {
            case false:

                erase_peer(s_master_unicast_mac.peer_addr);  

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
    memset(&send_param, 0, sizeof(slave_espnow_send_param_t));

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

    init_temperature_sensor();

    slave_espnow_init();

    xTaskCreate(slave_espnow_task, "slave_espnow_task", 4096, &send_param, 4, NULL);
}



    float angle=-100;
    float sin_angle1;
    float sin_angle2;
    float sin_angle3;
    float sin_angle4;
    #include <math.h>
void make_fake_data(){
    sin_angle1 =  sin(angle*0.06283)*10+10;
    sin_angle2 = sin(angle*0.031415)*10+10;
    sin_angle3 = sin(angle*0.0157)*10+10;
    sin_angle4 = sin(angle*0.031415)*10+30;

    angle++;
    if (angle>100){
        angle=-100;
    }
}

void prepare_data(sensor_data_t *data) {
    // Set MAC address (Example: "48:27:e2:c7:21:7c")
    uint8_t mac[6];
    esp_wifi_get_mac(ESP_IF_WIFI_STA, mac);
    memcpy(data->mac, mac, sizeof(mac));
    make_fake_data();
    data->temperature_mcu=read_internal_temperature_sensor();
    data->temperature_rdo = sin_angle1;
    data->do_value = sin_angle2;
    data->temperature_phg = sin_angle3;
    data->ph_value = sin_angle4;
}


#include "cJSON.h"
char* json_data(){
    cJSON *json_mac = cJSON_CreateObject();
    cJSON *json_data = cJSON_CreateObject();
    // Thêm các phần tử vào JSON object
    float temp_mcu=read_internal_temperature_sensor();
    char temp_mcu_str[6];
    snprintf(temp_mcu_str, sizeof(temp_mcu_str), "%.2f", temp_mcu);

    // temp_mcu=temp_mcu/100;
    uint8_t MAC_AP[6];
    esp_wifi_get_mac(ESP_IF_WIFI_STA, MAC_AP);
    char mac_str[18];
    // snprintf(mac_str, sizeof(mac_str), "%02X:%02X:%02X:%02X:%02X:%02X", 
    //          MAC_AP[0], MAC_AP[1], MAC_AP[2], MAC_AP[3], MAC_AP[4], MAC_AP[5]);
    snprintf(mac_str,18,MACSTR,MAC2STR(MAC_AP));
    cJSON_AddStringToObject(json_data, "mac", mac_str);
    cJSON_AddStringToObject(json_data, "tp_m", temp_mcu_str);
    cJSON_AddStringToObject(json_data, "slave_KEEP_connect", "");
    cJSON_AddStringToObject(json_data, "data", "30.44|50.83|90.65|10.23");
    char *json_string = cJSON_Print(json_data);
    return json_string;
}