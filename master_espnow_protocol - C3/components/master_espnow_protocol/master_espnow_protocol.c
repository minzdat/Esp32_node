#include "master_espnow_protocol.h"
// #include "read_serial.h"
#include "mbedtls/aes.h"

static int current_index = CURRENT_INDEX;
static const uint8_t s_master_broadcast_mac[ESP_NOW_ETH_ALEN] = MASTER_BROADCAST_MAC;
static QueueHandle_t s_master_espnow_queue;
static master_espnow_send_param_t send_param;
list_slaves_t test_allowed_connect_slaves[MAX_SLAVES];
list_slaves_t allowed_connect_slaves[MAX_SLAVES];
list_slaves_t waiting_connect_slaves[MAX_SLAVES];

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


// Function to save IP MAC Slave waiting to allow
void add_waiting_connect_slaves(const uint8_t *mac_addr) 
{
    for (int i = 0; i < MAX_SLAVES; i++) 
    {
        if (memcmp(waiting_connect_slaves[i].peer_addr, mac_addr, ESP_NOW_ETH_ALEN) == 0) 
        {
            // The MAC address already exists in the list, no need to add it
            ESP_LOGI(TAG, "MAC " MACSTR " is already in the waiting list", MAC2STR(mac_addr));
            return;
        }
    }
    
    // Find an empty spot to add a new MAC address
    for (int i = 0; i < MAX_SLAVES; i++) 
    {
        uint8_t zero_mac[ESP_NOW_ETH_ALEN] = {0};
        if (memcmp(waiting_connect_slaves[i].peer_addr, zero_mac, ESP_NOW_ETH_ALEN) == 0) 
        {
            // Empty location, add new MAC address here
            memcpy(waiting_connect_slaves[i].peer_addr, mac_addr, ESP_NOW_ETH_ALEN);
            ESP_LOGI(TAG, "Added MAC " MACSTR " to waiting allow connect list", MAC2STR(waiting_connect_slaves[i].peer_addr));
            ESP_LOGW(TAG, "Save waiting allow connect list into NVS");
            save_info_slaves_to_nvs("KEY_SLA_WAIT", waiting_connect_slaves); // Save to NVS
            return;
        }
    }
    
    // If the list is full, replace the MAC address at the current index in the ring list    
    memcpy(waiting_connect_slaves[current_index].peer_addr, mac_addr, ESP_NOW_ETH_ALEN);
    ESP_LOGW(TAG, "Waiting allow connect list is full, replaced MAC at index %d with " MACSTR, current_index, MAC2STR(mac_addr));

    // Update index for next addition
    current_index = (current_index + 1) % MAX_SLAVES;

    // Save updated list to NVS
    save_info_slaves_to_nvs("KEY_SLA_WAIT", waiting_connect_slaves);
}

/* Function responds with the specified MAC and content*/
esp_err_t response_specified_mac(const uint8_t *dest_mac, const char *message, bool encrypt)
{
    add_peer(dest_mac, encrypt);
  
    master_espnow_send_param_t send_param_agree;
    memset(&send_param_agree, 0, sizeof(master_espnow_send_param_t));

    send_param_agree.len = strlen(message);
    
    if (send_param_agree.len > sizeof(send_param_agree.buffer)) {
        ESP_LOGE(TAG, "Message too long to fit in the buffer");
        vSemaphoreDelete(s_master_espnow_queue);
        esp_now_deinit();
        return ESP_FAIL;
    }

    memcpy(send_param_agree.dest_mac, dest_mac, ESP_NOW_ETH_ALEN);
    memcpy(send_param_agree.buffer, message, send_param_agree.len);

    // Send the unicast response
    if (esp_now_send(send_param_agree.dest_mac, send_param_agree.buffer, send_param_agree.len) != ESP_OK) 
    {
        ESP_LOGE(TAG, "Send error");
        master_espnow_deinit(&send_param_agree);
        vTaskDelete(NULL);
    }

    return ESP_OK;
}

void master_espnow_send_cb(const uint8_t *mac_addr, esp_now_send_status_t status)
{
    master_espnow_event_t evt;
    master_espnow_event_send_cb_t *send_cb = &evt.info.send_cb;

    if (mac_addr == NULL) 
    {
        ESP_LOGE(TAG, "Send cb arg error");
        return;
    }

    evt.id = MASTER_ESPNOW_SEND_CB;
    memcpy(send_cb->mac_addr, mac_addr, ESP_NOW_ETH_ALEN);
    send_cb->status = status;
    // if (xQueueSend(s_master_espnow_queue, &evt, ESPNOW_MAXDELAY) != pdTRUE) {
    //     ESP_LOGW(TAG, "Send send queue fail");
    // }
    ESP_LOGI(TAG, "Send callback: MAC Address " MACSTR ", Status: %s",
        MAC2STR(mac_addr), (status == ESP_NOW_SEND_SUCCESS) ? "Success" : "Fail");
}


void master_espnow_recv_cb(const esp_now_recv_info_t *recv_info, const uint8_t *data, int len)
{
    master_espnow_event_t evt;
    master_espnow_event_recv_cb_t *recv_cb = &evt.info.recv_cb;
    uint8_t * mac_addr = recv_info->src_addr;
    uint8_t * des_addr = recv_info->des_addr;
    sensor_data_t *recv_data = (sensor_data_t *)data;

    if (mac_addr == NULL || data == NULL || len <= 0) 
    {
        ESP_LOGE(TAG, "Receive cb arg error");
        return;
    }

    evt.id = MASTER_ESPNOW_RECV_CB;
    memcpy(recv_cb->mac_addr, mac_addr, ESP_NOW_ETH_ALEN);

    if (len > MAX_DATA_LEN)
    {
        ESP_LOGE(TAG, "Received data length exceeds the maximum allowed");
        return;
    }

    recv_cb->data_len = len;
    memcpy(recv_cb->data, data, recv_cb->data_len);

    // if (xQueueSend(s_master_espnow_queue, &evt, ESPNOW_MAXDELAY) != pdTRUE) {
    //     ESP_LOGW(TAG, "Send receive queue fail");
    //     free(recv_cb->data);
    // }

    if (IS_BROADCAST_ADDR(des_addr)) 
    {
        // Log the MAC address, destination address, and received data
        ESP_LOGI(TAG, "_________________________________");
        ESP_LOGI(TAG, "Receive broadcast ESPNOW data");
        ESP_LOGI(TAG, "Received data from MAC: " MACSTR ", Data Length: %d, Data: %.*s",
            MAC2STR(recv_cb->mac_addr), recv_cb->data_len, recv_cb->data_len, recv_cb->data);       
        
        bool found = false;
        // Check if the source MAC address is in the allowed slaves list
        for (int i = 0; i < MAX_SLAVES; i++) 
        {
            if (memcmp(allowed_connect_slaves[i].peer_addr, recv_cb->mac_addr, ESP_NOW_ETH_ALEN) == 0) 
            {
                if (!allowed_connect_slaves[i].status)  //Slave in status Offline
                {
                    // Call a function to response agree connect
                    allowed_connect_slaves[i].start_time = esp_timer_get_time();
                    ESP_LOGW(TAG, "---------------------------------");
                    ESP_LOGW(TAG, "Response AGREE CONNECT to MAC " MACSTR "",  MAC2STR(recv_cb->mac_addr));
                    response_specified_mac(recv_cb->mac_addr, RESPONSE_AGREE_CONNECT, false);        
                }
                found = true;
                break;
            }
        }
        if (!found) 
        {
            // Call a function to add the slave to the waiting_connect_slaves list
            ESP_LOGW(TAG, "Add MAC " MACSTR " to WAITING CONNECT SLAVES LIST",  MAC2STR(recv_cb->mac_addr));
            add_waiting_connect_slaves(recv_cb->mac_addr);
        }  
    } 
    else 
    {
        ESP_LOGI(TAG, "_________________________________");
        ESP_LOGI(TAG, "Receive unicast ESPNOW data");
        // ESP_LOGI(TAG, "MAC " MACSTR " (length: %d): %.*s",MAC2STR(recv_cb->mac_addr), recv_cb->data_len, recv_cb->data_len, (char *)recv_cb->data);
        // Check if the received data is SLAVE_SAVED_MAC_MSG to change status of MAC Online
        for (int i = 0; i < MAX_SLAVES; i++) 
        {
            // Update the status of the corresponding MAC address in allowed_connect_slaves list
            if (memcmp(allowed_connect_slaves[i].peer_addr, recv_cb->mac_addr, ESP_NOW_ETH_ALEN) == 0) 
            {
                if (recv_cb->data_len >= strlen(SLAVE_SAVED_MAC_MSG) && memcmp(recv_cb->data, SLAVE_SAVED_MAC_MSG, recv_cb->data_len) == 0) 
                {
                    allowed_connect_slaves[i].status = true; // Set status to Online
                    ESP_LOGI(TAG, "Updated MAC " MACSTR " status to %s", MAC2STR(recv_cb->mac_addr),  allowed_connect_slaves[i].status ? "online" : "offline");
                    allowed_connect_slaves[i].start_time = 0;
                    allowed_connect_slaves[i].number_retry = 0;
                    break;
                }
                else if (recv_cb->data_len >= strlen(STILL_CONNECTED_MSG) && strstr((char *)recv_cb->data, STILL_CONNECTED_MSG) != NULL)
                {
                    allowed_connect_slaves[i].start_time = 0;
                    allowed_connect_slaves[i].check_connect_errors = 0;
                    allowed_connect_slaves[i].count_receive++;
                    ESP_LOGI(TAG, "MAC " MACSTR " (length: %d): %.*s",MAC2STR(recv_cb->mac_addr), recv_cb->data_len, recv_cb->data_len, (char *)recv_cb->data);
                    ESP_LOGW(TAG, "Receive from " MACSTR " - Counting: %d", MAC2STR(allowed_connect_slaves[i].peer_addr), allowed_connect_slaves[i].count_receive);
                    // dump_uart((const char *)recv_cb->data);
                    // dump_uart(recv_data);
                    break;
                }
                else{
                    allowed_connect_slaves[i].start_time = 0;
                    allowed_connect_slaves[i].check_connect_errors = 0;
                    allowed_connect_slaves[i].count_receive++;
                        // ESP_LOGW("","%s",(char *)recv_cb->data);
                        // recv_data->rssi=-5672367670;
                        ESP_LOGW(TAG, "MAC " MACSTR " (length: %d): %.*s",MAC2STR(recv_cb->mac_addr), recv_cb->data_len, recv_cb->data_len, (char *)recv_data);
                            ESP_LOGI("SENSOR_DATA", "Temperature MCU: %.2f", recv_data->temperature_mcu);
                            ESP_LOGI("SENSOR_DATA", "Temperature RDO: %.2f", recv_data->temperature_rdo);
                            ESP_LOGI("SENSOR_DATA", "Dissolved Oxygen: %.2f", recv_data->do_value);
                            ESP_LOGI("SENSOR_DATA", "Temperature PHG: %.2f", recv_data->temperature_phg);
                            ESP_LOGI("SENSOR_DATA", "pH: %.2f", recv_data->ph_value);
                            ESP_LOGI("SENSOR_DATA", "RSSI: %d", recv_data->rssi); // Chỉ in nếu có thành viên rssi
                        // dump_uart((const char *)recv_cb->data);
                        dump_uart(recv_data);

                        break;

                }

            }
        }
    }
}

void master_espnow_task(void *pvParameter)
{
    master_espnow_send_param_t *send_param = (master_espnow_send_param_t *)pvParameter;

    send_param->len = strlen(CHECK_CONNECTION_MSG);

    if (send_param->len > MAX_DATA_LEN) 
    {
        ESP_LOGE(TAG, "Message length exceeds buffer size");
        free(send_param);
        vSemaphoreDelete(s_master_espnow_queue);
        esp_now_deinit();
        return;
    }

    memcpy(send_param->buffer, CHECK_CONNECTION_MSG, send_param->len);

    while (true) 
    {
        read_internal_temperature_sensor();

        // Browse the allowed_connect_slaves list
        for (int i = 0; i < MAX_SLAVES; i++) 
        {
            if (allowed_connect_slaves[i].status) // Check online status
            { 
                add_peer(allowed_connect_slaves[i].peer_addr, true); 

                // Update destination MAC address
                memcpy(send_param->dest_mac, allowed_connect_slaves[i].peer_addr, ESP_NOW_ETH_ALEN);
                ESP_LOGW(TAG, "---------------------------------");
                ESP_LOGW(TAG, "Send to CHECK CONNECTION to MAC  " MACSTR "", MAC2STR(allowed_connect_slaves[i].peer_addr));

                // Count the number of packets sent
                allowed_connect_slaves[i].count_send ++;
                ESP_LOGW(TAG, "Send to " MACSTR " - Counting: %d", MAC2STR(allowed_connect_slaves[i].peer_addr), allowed_connect_slaves[i].count_send);

                if (esp_now_send(send_param->dest_mac, send_param->buffer, send_param->len) != ESP_OK) 
                {
                    allowed_connect_slaves[i].send_errors++;
                    ESP_LOGE(TAG, "Send error to MAC: " MACSTR ". Current send_errors count: %d", MAC2STR(send_param->dest_mac), allowed_connect_slaves[i].send_errors);
                    if (allowed_connect_slaves[i].send_errors >= MAX_SEND_ERRORS)
                    {
                        allowed_connect_slaves[i].status = false; // Mark MAC as offline
                        ESP_LOGW(TAG, "MAC: " MACSTR " has been marked offline", MAC2STR( allowed_connect_slaves[i].peer_addr));

                        // Reset value count if device marked offline
                        allowed_connect_slaves[i].count_receive = 0;
                        allowed_connect_slaves[i].count_send = 0;
                        allowed_connect_slaves[i].count_retry = 0;
                    }
                } 
                else 
                {
                    allowed_connect_slaves[i].send_errors = 0;
                    allowed_connect_slaves[i].start_time = esp_timer_get_time(); 
                }
            }
        }
        vTaskDelay(pdMS_TO_TICKS(TIME_CHECK_CONNECT));
    }
}

// Task Check the response from the Slaves
void retry_connect_lost_task(void *pvParameter)
{
    while (true) 
    {
        for (int i = 0; i < MAX_SLAVES; i++) 
        {
            switch (allowed_connect_slaves[i].status) 
            {
                case false: // Slave Offline
                    if(allowed_connect_slaves[i].start_time > 0)
                    {
                        allowed_connect_slaves[i].end_time =  esp_timer_get_time();
                        uint64_t elapsed_time = allowed_connect_slaves[i].end_time - allowed_connect_slaves[i].start_time;

                        if (elapsed_time > 5 * 1000000) 
                        {
                            if (allowed_connect_slaves[i].number_retry == NUMBER_RETRY)
                            {
                                ESP_LOGW(TAG, "---------------------------------");
                                ESP_LOGW(TAG, "Retry AGREE CONNECT with " MACSTR "", MAC2STR(allowed_connect_slaves[i].peer_addr));
                                response_specified_mac(allowed_connect_slaves[i].peer_addr, RESPONSE_AGREE_CONNECT, false);        
                                allowed_connect_slaves[i].number_retry++;
                            }
                            else
                            {
                                allowed_connect_slaves[i].start_time =  0;
                                allowed_connect_slaves[i].number_retry = 0;
                            }
                        }
                    }
                    break;

                case true: // Slave Online
                    if(allowed_connect_slaves[i].start_time > 0)
                    {
                        allowed_connect_slaves[i].end_time =  esp_timer_get_time();
                        uint64_t elapsed_time = allowed_connect_slaves[i].end_time - allowed_connect_slaves[i].start_time;

                        if (elapsed_time > 5 * 1000000) 
                        {
                            allowed_connect_slaves[i].check_connect_errors++;
                            allowed_connect_slaves[i].count_retry++;

                            ESP_LOGW(TAG, "---------------------------------");
                            ESP_LOGW(TAG, "Retry CHECK CONNECT with " MACSTR " Current count retry: %d", MAC2STR(allowed_connect_slaves[i].peer_addr), allowed_connect_slaves[i].count_retry);               
                            response_specified_mac(allowed_connect_slaves[i].peer_addr, CHECK_CONNECTION_MSG, true);    

                            if (allowed_connect_slaves[i].check_connect_errors == NUMBER_RETRY)
                            {
                                allowed_connect_slaves[i].status = false;
                                ESP_LOGW(TAG, "Change " MACSTR " has been marked offline", MAC2STR(allowed_connect_slaves[i].peer_addr));

                                allowed_connect_slaves[i].start_time = 0;
                                allowed_connect_slaves[i].check_connect_errors = 0;
                            }
                        }
                    }
                    break;
            }
        }
        vTaskDelay(pdMS_TO_TICKS(1000)); // Delay 1s
    }
}

esp_err_t master_espnow_init(void)
{
    s_master_espnow_queue = xQueueCreate(ESPNOW_QUEUE_SIZE, sizeof(master_espnow_event_t));
    if (s_master_espnow_queue == NULL) 
    {
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
    /* Set primary master key. */
    ESP_ERROR_CHECK( esp_now_set_pmk((uint8_t *)CONFIG_ESPNOW_PMK) ); 
    
    /* Initialize sending parameters. */
    memset(&send_param, 0, sizeof(master_espnow_send_param_t));

    return ESP_OK;
}

void master_espnow_deinit(master_espnow_send_param_t *send_param)
{
    free(send_param->buffer);
    free(send_param);
    vSemaphoreDelete(s_master_espnow_queue);
    esp_now_deinit();
}

void master_espnow_protocol()
{
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) 
    {
        ESP_ERROR_CHECK( nvs_flash_erase() );
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK( ret );

    master_wifi_init();

    init_temperature_sensor();

    // ---Data demo MAC from Slave---

    test_allowed_connect_slaves_to_nvs(test_allowed_connect_slaves);

    save_info_slaves_to_nvs("KEY_SLA_ALLOW", test_allowed_connect_slaves);

    load_info_slaves_from_nvs("KEY_SLA_ALLOW", allowed_connect_slaves);

    // ---Data demo MAC from Slave---

    master_espnow_init();

    xTaskCreate(master_espnow_task, "master_espnow_task", 4096, &send_param, 4, NULL);

    xTaskCreate(retry_connect_lost_task, "retry_connect_lost_task", 4096, NULL, 5, NULL);
}