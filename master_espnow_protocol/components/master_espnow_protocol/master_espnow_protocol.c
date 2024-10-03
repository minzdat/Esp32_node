#include "master_espnow_protocol.h"

int64_t start_time_send_espnow = 0;
int devices_online = 0;
int64_t start_time_check_connect;
static int count_retry_send_callback;
static int current_index = CURRENT_INDEX;
static int8_t rssi;
static char message_packed[MAX_PAYLOAD_LEN]; 
static char saved_message_check_connect[PACKED_MSG_SIZE];
static const uint8_t s_master_broadcast_mac[ESP_NOW_ETH_ALEN] = MASTER_BROADCAST_MAC;
static uint16_t s_espnow_seq[ESPNOW_DATA_MAX] = { 0, 0 };
static master_espnow_send_param_t send_param;
static master_espnow_send_param_t send_param_specified;
list_slaves_t test_allowed_connect_slaves[MAX_SLAVES];
list_slaves_t allowed_connect_slaves[MAX_SLAVES];
list_slaves_t waiting_connect_slaves[MAX_SLAVES];
sensor_data_t esp_data_sensor;
table_device_t table_devices[MAX_SLAVES];
SemaphoreHandle_t table_devices_mutex;
EventGroupHandle_t xEventGroup;
EventGroupHandle_t xEventGroupLightSleep;
QueueHandle_t slave_disconnect_queue;
TaskHandle_t master_espnow_handle = NULL;
TaskHandle_t retry_connect_lost_handle = NULL;

void log_send_espnow_result(esp_err_t result) 
{
    if (result == ESP_OK) 
    {
        ESP_LOGI(TAG, "Send Success");
    } 
    else if (result == ESP_ERR_ESPNOW_NOT_INIT) 
    {
        ESP_LOGE(TAG, "ESPNOW not initialized");
    } 
    else if (result == ESP_ERR_ESPNOW_ARG) 
    {
        ESP_LOGE(TAG, "Invalid argument");
    } 
    else if (result == ESP_ERR_ESPNOW_INTERNAL) 
    {
        ESP_LOGE(TAG, "Internal error");
    } 
    else if (result == ESP_ERR_ESPNOW_NO_MEM) 
    {
        ESP_LOGE(TAG, "Out of memory");
    } 
    else if (result == ESP_ERR_ESPNOW_NOT_FOUND) 
    {
        ESP_LOGE(TAG, "Peer is not found");
    } 
    else if (result == ESP_ERR_ESPNOW_IF) 
    {
        ESP_LOGE(TAG, "Current Wi-Fi interface doesn't match that of peer");
    } 
    else if (result == ESP_ERR_ESPNOW_CHAN) 
    {
        ESP_LOGE(TAG, "Current Wi-Fi channel doesn't match that of peer");
    } 
    else 
    {
        ESP_LOGE(TAG, "Unknown error code: %d", result);
    }
}

void erase_table_devices(int i) 
{
    if (xSemaphoreTake(table_devices_mutex, portMAX_DELAY)) 
    {
        memset(&table_devices[i], 0, sizeof(table_device_t));
        ESP_LOGE(TAG, "Erase Table Devices at index %d", i);
        log_table_devices();

        xSemaphoreGive(table_devices_mutex);
    } 
    else 
    {
        ESP_LOGE(TAG, "Failed to take mutex to erase device from table_devices");
    }
}

void log_table_devices() 
{
    ESP_LOGI(TAG, "-------------------------------------------------------------------------------------------------------------------");
        ESP_LOGI(TAG, "| %-17s | %-7s | %-7s | %-12s | %-12s | %-12s | %-8s | %-8s | %-7s |", 
                 "MAC Address", "Status", "RSSI", "Temp MCU", "Temp RDO", "Temp PHG", "DO Value", "pH Value", "Relay");
        ESP_LOGI(TAG, "------------------------------------------------------------------------------------------------------------------");
        
        for (int i = 0; i < MAX_SLAVES; i++) 
        {
            if (memcmp(table_devices[i].peer_addr, "\0\0\0\0\0\0", ESP_NOW_ETH_ALEN) != 0) 
            {
                char mac_str[18];
                sprintf(mac_str, "%02X:%02X:%02X:%02X:%02X:%02X", 
                        table_devices[i].peer_addr[0], table_devices[i].peer_addr[1], table_devices[i].peer_addr[2],
                        table_devices[i].peer_addr[3], table_devices[i].peer_addr[4], table_devices[i].peer_addr[5]);

                ESP_LOGI(TAG, "| %-17s | %-7s | %-7d | %-12.2f | %-12.2f | %-12.2f | %-8.2f | %-8.2f | %-7s |",
                         mac_str,
                         table_devices[i].status ? "Online" : "Offline",
                         table_devices[i].data.rssi,
                         table_devices[i].data.temperature_mcu,
                         table_devices[i].data.temperature_rdo,
                         table_devices[i].data.temperature_phg,
                         table_devices[i].data.do_value,
                         table_devices[i].data.ph_value,
                         table_devices[i].data.relay_state ? "On" : "Off");
            }
        }

    ESP_LOGI(TAG, "------------------------------------------------------------------------------------------------------------------");
}

void write_table_devices(const uint8_t *peer_addr, const sensor_data_t *esp_data, bool status) 
{
    if (xSemaphoreTake(table_devices_mutex, portMAX_DELAY)) 
    {
        for (int i = 0; i < MAX_SLAVES; i++) 
        {
            // Check if the MAC address already exists in the table
            if (memcmp(table_devices[i].peer_addr, peer_addr, ESP_NOW_ETH_ALEN) == 0) 
            {
                // Update data if it already exists
                table_devices[i].status = status;

                // Only update data if esp_data is not NULL
                if (esp_data != NULL) 
                {
                    table_devices[i].data = *esp_data;
                }
                
                log_table_devices();

                // Free the mutex
                xSemaphoreGive(table_devices_mutex);
                return;
            }
        }

        // If the MAC address does not exist, add it to the table
        for (int i = 0; i < MAX_SLAVES; i++) 
        {
            // Find blank cells (blank MAC address)
            if (memcmp(table_devices[i].peer_addr, "\0\0\0\0\0\0", ESP_NOW_ETH_ALEN) == 0) 
            {
                // Copy MAC address and save data
                memcpy(table_devices[i].peer_addr, peer_addr, ESP_NOW_ETH_ALEN);
                table_devices[i].status = status;

                // Only update data if esp_data is not NULL
                if (esp_data != NULL) 
                {
                    table_devices[i].data = *esp_data;
                }
                
                log_table_devices();
                
                // Free the mutex
                xSemaphoreGive(table_devices_mutex);
                return;
            }
        }

        ESP_LOGW(TAG, "Table device is full. Cannot add new device");

        log_table_devices();

        // Free the mutex
        xSemaphoreGive(table_devices_mutex);
    }
    else 
    {
        ESP_LOGE(TAG, "Failed to take mutex to save data to table_devices");
    }
}

void prepare_payload(espnow_data_t *espnow_data, float temperature_mcu, int rssi, float temperature_rdo, float do_value, float temperature_phg, float ph_value, bool relay_state) 
{
    // Initialize sensor data directly in the payload field
    espnow_data->payload.temperature_mcu = temperature_mcu;
    espnow_data->payload.rssi = rssi;
    espnow_data->payload.temperature_rdo = temperature_rdo;
    espnow_data->payload.do_value = do_value;
    espnow_data->payload.temperature_phg = temperature_phg;
    espnow_data->payload.ph_value = ph_value;
    espnow_data->payload.relay_state = relay_state;

    // Print payload size and data for testing
    ESP_LOGI(TAG, "     Payload size: %d bytes", sizeof(sensor_data_t));
    ESP_LOGI(TAG, "         MCU Temperature: %.2f", espnow_data->payload.temperature_mcu);
    ESP_LOGI(TAG, "         RSSI: %d", espnow_data->payload.rssi);
    ESP_LOGI(TAG, "         RDO Temperature: %.2f", espnow_data->payload.temperature_rdo);
    ESP_LOGI(TAG, "         DO Value: %.2f", espnow_data->payload.do_value);
    ESP_LOGI(TAG, "         PHG Temperature: %.2f", espnow_data->payload.temperature_phg);
    ESP_LOGI(TAG, "         PH Value: %.2f", espnow_data->payload.ph_value);
    ESP_LOGI(TAG, "         Relay State: %s", espnow_data->payload.relay_state ? "On" : "Off");
}

/* Parse ESPNOW data payload. */
void parse_payload(espnow_data_t *espnow_data) 
{
    // Save values ​​to esp_data_sensor
    esp_data_sensor.temperature_mcu = espnow_data->payload.temperature_mcu;
    esp_data_sensor.rssi = espnow_data->payload.rssi;
    esp_data_sensor.temperature_rdo = espnow_data->payload.temperature_rdo;
    esp_data_sensor.do_value = espnow_data->payload.do_value;
    esp_data_sensor.temperature_phg = espnow_data->payload.temperature_phg;
    esp_data_sensor.ph_value = espnow_data->payload.ph_value;
    esp_data_sensor.relay_state = espnow_data->payload.relay_state;

    // Directly access the fields of the payload
    ESP_LOGI(TAG, "     Parsed ESPNOW payload:");
    ESP_LOGI(TAG, "         MCU Temperature: %.2f", espnow_data->payload.temperature_mcu);
    ESP_LOGI(TAG, "         RSSI: %d", espnow_data->payload.rssi);
    ESP_LOGI(TAG, "         RDO Temperature: %.2f", espnow_data->payload.temperature_rdo);
    ESP_LOGI(TAG, "         DO Value: %.2f", espnow_data->payload.do_value);
    ESP_LOGI(TAG, "         PHG Temperature: %.2f", espnow_data->payload.temperature_phg);
    ESP_LOGI(TAG, "         PH Value: %.2f", espnow_data->payload.ph_value);
    ESP_LOGI(TAG, "         Relay State: %s", espnow_data->payload.relay_state ? "On" : "Off");

}

/* Prepare ESPNOW data to be sent. */
void espnow_data_prepare(master_espnow_send_param_t *send_param, const char *message)
{
    espnow_data_t *buf = (espnow_data_t *)send_param->buffer;

    assert(send_param->len >= sizeof(espnow_data_t));

    buf->type = IS_BROADCAST_ADDR(send_param->dest_mac) ? ESPNOW_DATA_BROADCAST : ESPNOW_DATA_UNICAST;
    buf->seq_num = s_espnow_seq[buf->type]++;
    buf->crc = 0;
    
    // Calculate the size of the message
    size_t message_size = strlen(message);

    // Copy message to sensor_data, ensuring not to exceed the allocated size
    strncpy(buf->message, message, message_size);
    buf->message[message_size] = '\0';

    // Lưu lại message vào biến saved_message
    strncpy(saved_message_check_connect, message, message_size);
    saved_message_check_connect[message_size] = '\0';

    // Log the data received
    ESP_LOGI(TAG, "Parsed ESPNOW packed:");
    ESP_LOGI(TAG, "     type: %d", buf->type);
    ESP_LOGI(TAG, "     seq_num: %d", buf->seq_num);
    ESP_LOGI(TAG, "     crc: %d", buf->crc);
    ESP_LOGI(TAG, "     message: %s", buf->message);

    // float temperature = read_internal_temperature_sensor();
    // prepare_payload(buf, temperature, rssi, 23.1, 7.6, 24.0, 7.2, esp_data_sensor.relay_state);

    buf->crc = esp_crc16_le(UINT16_MAX, (uint8_t const *)buf, send_param->len);
}

/* Parse received ESPNOW data. */
void espnow_data_parse(uint8_t *data, uint16_t data_len)
{
    espnow_data_t *buf = (espnow_data_t *)data;
    uint16_t crc, crc_cal = 0;

    if (data_len < sizeof(espnow_data_t)) 
    {
        ESP_LOGE(TAG, "Receive ESPNOW data too short, len:%d", data_len);
        return;
    }
    
    // Log the data received
    ESP_LOGI(TAG, "Parsed ESPNOW packed:");
    ESP_LOGI(TAG, "     type: %d", buf->type);
    ESP_LOGI(TAG, "     seq_num: %d", buf->seq_num);
    ESP_LOGI(TAG, "     crc: %d", buf->crc);

    // Copy the full message array safely
    memcpy(message_packed, buf->message, sizeof(buf->message));
    ESP_LOGI(TAG, "     message: %s", message_packed);

    // Log the payload if present
    if (data_len > sizeof(espnow_data_t)) 
    {
        parse_payload(buf);
    } 
    else 
    {
        ESP_LOGI(TAG, "  No payload data.");
    }

    crc = buf->crc;
    buf->crc = 0;
    crc_cal = esp_crc16_le(UINT16_MAX, (uint8_t const *)buf, data_len);

    if (crc_cal == crc) 
    {
        ESP_LOGI(TAG, "CRC check passed.");
    } 
    else 
    {
        ESP_LOGE(TAG, "CRC check failed. Calculated CRC: %d, Received CRC: %d", crc_cal, crc);
        return;
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

void erase_peer(const uint8_t *peer_mac) 
{
    if (esp_now_is_peer_exist(peer_mac)) 
    {
        esp_err_t del_err = esp_now_del_peer(peer_mac);
        if (del_err != ESP_OK) 
        {
            ESP_LOGE(TAG, "Failed to delete peer " MACSTR ": %s", MAC2STR(peer_mac), esp_err_to_name(del_err));
        } 
        else 
        {
            ESP_LOGI(TAG, "Deleted peer " MACSTR, MAC2STR(peer_mac));
        }
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
            ESP_LOGW(TAG, "Save WAITING_CONNECT_SLAVES_LIST into NVS");
            save_info_slaves_to_nvs("KEY_SLA_WAIT", waiting_connect_slaves); // Save to NVS

            // erase_key_in_nvs("KEY_SLA_ALLOW");
            // memset(allowed_connect_slaves, 0, sizeof(allowed_connect_slaves));

            // Callback function send WAITING_CONNECT_SLAVES_LIST to S3

            return;
        }
    }
    
    // If the list is full, replace the MAC address at the current index in the ring list    
    memcpy(waiting_connect_slaves[current_index].peer_addr, mac_addr, ESP_NOW_ETH_ALEN);
    ESP_LOGW(TAG, "WAITING_CONNECT_SLAVES_LIST is full, replaced MAC at index %d with " MACSTR, current_index, MAC2STR(mac_addr));

    // Update index for next addition
    current_index = (current_index + 1) % MAX_SLAVES;

    // Save updated list to NVS
    save_info_slaves_to_nvs("KEY_SLA_WAIT", waiting_connect_slaves);

    // erase_key_in_nvs("KEY_SLA_ALLOW");
    // memset(allowed_connect_slaves, 0, sizeof(allowed_connect_slaves));

    // Callback function send WAITING_CONNECT_SLAVES_LIST to S3
}

/* Function responds with the specified MAC and content*/
esp_err_t response_specified_mac(const uint8_t *dest_mac, const char *message)
{
    send_param_specified.len = MAX_DATA_LEN;
    memcpy(send_param_specified.dest_mac, dest_mac, ESP_NOW_ETH_ALEN);

    espnow_data_prepare(&send_param_specified, message);

    int64_t elapsed_time_us = esp_timer_get_time() - start_time_send_espnow;
    int64_t remaining_time = 100000 - elapsed_time_us;

    if (remaining_time > 0 && count_retry_send_callback >= 2)
    {
        ESP_LOGI(TAG, "Remaining time to wait before retry_send_callback: %lld us", remaining_time);
        vTaskDelay(pdMS_TO_TICKS(remaining_time / 1000));
    }

    int64_t elapsed_time_enter_retry = esp_timer_get_time() - start_time_send_espnow;
    ESP_LOGI(TAG, "Time between each retry_send_callback : %lld us", (elapsed_time_enter_retry));

    start_time_send_espnow = esp_timer_get_time();

    esp_err_t ret_val = esp_now_send(send_param_specified.dest_mac, send_param_specified.buffer, send_param_specified.len);
    log_send_espnow_result(ret_val);

    if (ret_val != ESP_OK) 
    {
        ESP_LOGE(TAG, "Send error");
        // master_espnow_deinit();
        // vTaskDelete(NULL);
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

    ESP_LOGI(TAG, "Send callback: MAC Address " MACSTR ", Status: %s",
        MAC2STR(mac_addr), (status == ESP_NOW_SEND_SUCCESS) ? "Success" : "Fail");
    
    // Check if the received data is SLAVE_SAVED_MAC_MSG to change status of MAC Online
    if (status != ESP_NOW_SEND_SUCCESS)
    {
        for (int i = 0; i < MAX_SLAVES; i++) 
        {
            if ((memcmp(allowed_connect_slaves[i].peer_addr, mac_addr, ESP_NOW_ETH_ALEN) == 0) &&  
            (strcmp(allowed_connect_slaves[i].message_retry_fail, CHECK_CONNECTION_MSG) == 0))
            {
                if (allowed_connect_slaves[i].count_retry < SEND_CALLBACK_RETRY)
                {
                    allowed_connect_slaves[i].count_retry++; 
                    count_retry_send_callback = allowed_connect_slaves[i].count_retry;
                    ESP_LOGW(TAG, "Number of retry_send_callback to MAC  " MACSTR " | : %d", MAC2STR(mac_addr), allowed_connect_slaves[i].count_retry);

                    response_specified_mac(allowed_connect_slaves[i].peer_addr, allowed_connect_slaves[i].message_retry_fail);     
                }
                else if ((allowed_connect_slaves[i].count_retry == SEND_CALLBACK_RETRY))
                {
                    allowed_connect_slaves[i].check_connect_errors++;
                    ESP_LOGW(TAG, "Number of check_connection to MAC  " MACSTR " | : %d", MAC2STR(mac_addr), allowed_connect_slaves[i].check_connect_errors);


                    ESP_LOGI(TAG, "Continue signal received, resuming task...");
                    xEventGroupSetBits(xEventGroup, EVENT_BIT_CONTINUE);
                    xQueueSend(slave_disconnect_queue, &i, portMAX_DELAY);
                    xEventGroupSetBits(xEventGroupLightSleep, (1 << i));
                }
                return;
            }
        }
    }
    else if (status == ESP_NOW_SEND_SUCCESS)
    {
        for (int i = 0; i < MAX_SLAVES; i++) 
        {
            if (memcmp(allowed_connect_slaves[i].peer_addr, mac_addr, ESP_NOW_ETH_ALEN) == 0)
            {
                if ((strcmp(allowed_connect_slaves[i].message_retry_fail, CHECK_CONNECTION_MSG) == 0) || (strcmp(saved_message_check_connect, CHECK_CONNECTION_MSG) == 0))
                {
                    allowed_connect_slaves[i].check_keep_connect = true;
                    memcpy(allowed_connect_slaves[i].message_retry_fail, NOT_RETRY_MSG, sizeof(NOT_RETRY_MSG));
                    allowed_connect_slaves[i].count_retry = 0;

                    count_retry_send_callback = allowed_connect_slaves[i].count_retry;
                    allowed_connect_slaves[i].check_connect_errors++;
                    ESP_LOGW(TAG, "Number of check_connection to MAC  " MACSTR "| : %d", MAC2STR(mac_addr), allowed_connect_slaves[i].check_connect_errors);

                    ESP_LOGI(TAG, "Continue signal received, resuming task...");
                    xEventGroupSetBits(xEventGroup, EVENT_BIT_CONTINUE);
                    xEventGroupSetBits(xEventGroupLightSleep, (1 << i));
                }
                return;
            }
        }
    }
}

void master_espnow_recv_cb(const esp_now_recv_info_t *recv_info, const uint8_t *data, int len)
{
    rssi = recv_info->rx_ctrl->rssi;

    master_espnow_event_t evt;
    master_espnow_event_recv_cb_t *recv_cb = &evt.info.recv_cb;
    uint8_t * mac_addr = recv_info->src_addr;
    uint8_t * des_addr = recv_info->des_addr;

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

    if (IS_BROADCAST_ADDR(des_addr)) 
    {   
        ESP_LOGI(TAG, "_________________________________");
        ESP_LOGI(TAG, "Receive broadcast ESPNOW data");

        bool found = false;
        // Check if the source MAC address is in the allowed slaves list
        for (int i = 0; i < MAX_SLAVES; i++) 
        {
            if (memcmp(allowed_connect_slaves[i].peer_addr, recv_cb->mac_addr, ESP_NOW_ETH_ALEN) == 0) 
            {
                if (!allowed_connect_slaves[i].status)  //Slave in status Offline
                {
                    // Parse the received data when the condition is met
                    espnow_data_parse(recv_cb->data, recv_cb->data_len);

                    if (recv_cb->data_len >= strlen(REQUEST_CONNECTION_MSG) && strstr((char *)message_packed, REQUEST_CONNECTION_MSG) != NULL) 
                    {
                        // Call a function to response agree connect
                        allowed_connect_slaves[i].start_time = esp_timer_get_time();
                        ESP_LOGW(TAG, "---------------------------------");
                        ESP_LOGW(TAG, "Response %s to MAC " MACSTR "",RESPONSE_AGREE_CONNECT,  MAC2STR(recv_cb->mac_addr));

                        add_peer(recv_cb->mac_addr, false);
                        response_specified_mac(recv_cb->mac_addr, RESPONSE_AGREE_CONNECT);
                    }        
                }
                found = true;
                break;
            }
        }
        if (!found) 
        {
            // Call a function to add the slave to the waiting_connect_slaves list
            ESP_LOGW(TAG, "Add MAC " MACSTR " to WAITING_CONNECT_SLAVES_LIST",  MAC2STR(recv_cb->mac_addr));
            add_waiting_connect_slaves(recv_cb->mac_addr);
        }  
    } 
    else 
    {  
        ESP_LOGI(TAG, "_________________________________");
        ESP_LOGI(TAG, "Receive unicast ESPNOW data");
        ESP_LOGW(TAG, "Receive unicast MAC " MACSTR "", MAC2STR(recv_cb->mac_addr));

        // Check if the received data is SLAVE_SAVED_MAC_MSG to change status of MAC Online
        for (int i = 0; i < MAX_SLAVES; i++) 
        {
            // Update the status of the corresponding MAC address in allowed_connect_slaves list
            if (memcmp(allowed_connect_slaves[i].peer_addr, recv_cb->mac_addr, ESP_NOW_ETH_ALEN) == 0) 
            {
                // Parse the received data when the condition is met
                espnow_data_parse(recv_cb->data, recv_cb->data_len);

                if (recv_cb->data_len >= strlen(SLAVE_SAVED_MAC_MSG) && strstr((char *)message_packed, SLAVE_SAVED_MAC_MSG) != NULL) 
                {
                    allowed_connect_slaves[i].status = true; // Set status to Online
                    allowed_connect_slaves[i].check_keep_connect = false;
                    if (devices_online <= MAX_SLAVES)
                    {
                        all_slaves_bits |= (1 << i); 
                        devices_online++;
                    }

                    ESP_LOGI(TAG, "Updated MAC " MACSTR " status to %s", MAC2STR(recv_cb->mac_addr),  allowed_connect_slaves[i].status ? "online" : "offline");

                    allowed_connect_slaves[i].start_time = 0;
                    allowed_connect_slaves[i].number_retry = 0;

                    save_info_slaves_to_nvs("KEY_SLA_ALLOW", allowed_connect_slaves);
                    write_table_devices(allowed_connect_slaves[i].peer_addr, NULL, allowed_connect_slaves[i].status);

                    add_peer(recv_cb->mac_addr, true);

                    break;
                }
                else if (recv_cb->data_len >= strlen(STILL_CONNECTED_MSG) && strstr((char *)message_packed, STILL_CONNECTED_MSG) != NULL)
                {
                    allowed_connect_slaves[i].check_keep_connect = false;
                    memcpy(allowed_connect_slaves[i].message_retry_fail, NOT_RETRY_MSG, sizeof(NOT_RETRY_MSG));

                    write_table_devices(allowed_connect_slaves[i].peer_addr, &esp_data_sensor, allowed_connect_slaves[i].status);

                    allowed_connect_slaves[i].start_time = 0;
                    allowed_connect_slaves[i].check_connect_errors = 0;
                    allowed_connect_slaves[i].count_retry = 0;

                    count_retry_send_callback = allowed_connect_slaves[i].count_retry;

                    break;
                }
                else if (recv_cb->data_len >= strlen(CONTROL_RELAY_MSG) && strstr((char *)message_packed, CONTROL_RELAY_MSG) != NULL)
                {

                }
                else if (recv_cb->data_len >= strlen(DISCONNECT_NODE_MSG) && strstr((char *)message_packed, DISCONNECT_NODE_MSG) != NULL)
                {
                    // Clear the `allowed_connect_slaves` array
                    memset(&allowed_connect_slaves[i], 0, sizeof(list_slaves_t));
                    erase_table_devices(i);
                }
            }
        }
    }
}

// Task Check connect to Slaves
void master_espnow_task(void *pvParameter)
{
    memset(&send_param, 0, sizeof(master_espnow_send_param_t));
    master_espnow_send_param_t *send_param = (master_espnow_send_param_t *)pvParameter;
    send_param->len = MAX_DATA_LEN;

    start_time_check_connect = esp_timer_get_time();

    while (true) 
    {
        ESP_LOGE(TAG, "Task master_espnow_task");
        //Send check connect
        int64_t current_time = esp_timer_get_time();
        if ((current_time - start_time_check_connect) >= TIME_CHECK_CONNECT) 
        {
            // Browse the allowed_connect_slaves list
            for (int i = 0; i < MAX_SLAVES; i++) 
            {
                if (allowed_connect_slaves[i].status) // Check online status
                {
                    if (allowed_connect_slaves[i].check_connect_errors <= NUMBER_RETRY)
                    {   
                        allowed_connect_slaves[i].count_retry = 0;
                        count_retry_send_callback = allowed_connect_slaves[i].count_retry;

                        // Copy the full message array safely
                        memcpy(allowed_connect_slaves[i].message_retry_fail, CHECK_CONNECTION_MSG, sizeof(CHECK_CONNECTION_MSG));

                        // Update destination MAC address
                        memcpy(send_param->dest_mac, allowed_connect_slaves[i].peer_addr, ESP_NOW_ETH_ALEN);
                        ESP_LOGW(TAG, "---------------------------------");
                        ESP_LOGW(TAG, "Send %s to MAC  " MACSTR "",CHECK_CONNECTION_MSG, MAC2STR(allowed_connect_slaves[i].peer_addr));

                        //Prepare date before send                    
                        espnow_data_prepare(send_param, CHECK_CONNECTION_MSG); 

                        // Send check connect to slave vs espnow 
                        esp_err_t ret_val = esp_now_send(send_param->dest_mac, send_param->buffer, send_param->len);
                        log_send_espnow_result(ret_val);
                        if (ret_val != ESP_OK) 
                        {
                            allowed_connect_slaves[i].send_errors++;
                            ESP_LOGE(TAG, "Send error to MAC: " MACSTR ". Current send_errors count: %d", MAC2STR(send_param->dest_mac), allowed_connect_slaves[i].send_errors);
                            
                            if (allowed_connect_slaves[i].send_errors >= MAX_SEND_ERRORS)
                            {
                                all_slaves_bits &= ~(1 << i);
                                allowed_connect_slaves[i].status = false; // Mark MAC as offline
                                devices_online--;

                                ESP_LOGW(TAG, "MAC: " MACSTR " has been marked offline", MAC2STR( allowed_connect_slaves[i].peer_addr));

                                save_info_slaves_to_nvs("KEY_SLA_ALLOW", allowed_connect_slaves);
                                write_table_devices(allowed_connect_slaves[i].peer_addr, NULL, allowed_connect_slaves[i].status);
                                erase_peer(allowed_connect_slaves[i].peer_addr);
                            }
                        } 
                        else 
                        {
                            allowed_connect_slaves[i].send_errors = 0;
                            allowed_connect_slaves[i].start_time = esp_timer_get_time(); 
                        
                            ESP_LOGI(TAG, "Task is paused, waiting for continue signal...");
                            xEventGroupWaitBits(xEventGroup, EVENT_BIT_CONTINUE, pdFALSE, pdFALSE, portMAX_DELAY);
                            xEventGroupClearBits(xEventGroup, EVENT_BIT_CONTINUE);
                        }
                    }
                }
            }
            start_time_check_connect = esp_timer_get_time();
        }
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

// Task Check the response from the Slaves
void retry_connect_lost_task(void *pvParameter)
{
    while (true) 
    {
        ESP_LOGE(TAG, "Task retry_connect_lost_task");
        for (int i = 0; i < MAX_SLAVES; i++) 
        {
            switch (allowed_connect_slaves[i].status) 
            {
                case false: // Slave Offline
                    if(allowed_connect_slaves[i].start_time > 0)
                    {
                        allowed_connect_slaves[i].end_time =  esp_timer_get_time();
                        uint64_t elapsed_time = allowed_connect_slaves[i].end_time - allowed_connect_slaves[i].start_time;
                        if (elapsed_time > RETRY_TIMEOUT) 
                        {
                            if (allowed_connect_slaves[i].number_retry >= NUMBER_RETRY)
                            {
                                ESP_LOGW(TAG, "---------------------------------");
                                ESP_LOGW(TAG, "Retry %s with " MACSTR "",RESPONSE_AGREE_CONNECT, MAC2STR(allowed_connect_slaves[i].peer_addr));
                                response_specified_mac(allowed_connect_slaves[i].peer_addr, RESPONSE_AGREE_CONNECT);        
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
                    uint32_t slave_index;
                    while (xQueueReceive(slave_disconnect_queue, &slave_index, pdMS_TO_TICKS(100)) == pdTRUE)
                    {
                        all_slaves_bits &= ~(1 << i);
                        allowed_connect_slaves[slave_index].status = false;
                        devices_online--;
                        allowed_connect_slaves[slave_index].check_connect_success = false;
                        allowed_connect_slaves[slave_index].check_connect_errors = 0;
                        allowed_connect_slaves[slave_index].check_keep_connect = false;

                        allowed_connect_slaves[slave_index].count_retry = 0;

                        count_retry_send_callback = allowed_connect_slaves[slave_index].count_retry;
                        memcpy(allowed_connect_slaves[slave_index].message_retry_fail, NOT_RETRY_MSG, sizeof(NOT_RETRY_MSG));

                        allowed_connect_slaves[slave_index].start_time = 0;

                        ESP_LOGW(TAG, "Change " MACSTR " has been marked offline", MAC2STR(allowed_connect_slaves[slave_index].peer_addr));

                        save_info_slaves_to_nvs("KEY_SLA_ALLOW", allowed_connect_slaves);
                        write_table_devices(allowed_connect_slaves[slave_index].peer_addr, NULL, allowed_connect_slaves[slave_index].status);
                        erase_peer(allowed_connect_slaves[slave_index].peer_addr);
                    }
                    break;
            }
        }
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

esp_err_t master_espnow_init(void)
{
    /* Initialize ESPNOW and register sending and receiving callback function. */
    ESP_ERROR_CHECK( esp_now_init() );
    ESP_ERROR_CHECK( esp_now_register_send_cb(master_espnow_send_cb) );
    ESP_ERROR_CHECK( esp_now_register_recv_cb(master_espnow_recv_cb) );
#if CONFIG_ESPNOW_ENABLE_POWER_SAVE
    ESP_ERROR_CHECK( esp_now_set_wake_window(CONFIG_ESPNOW_WAKE_WINDOW) );
    ESP_ERROR_CHECK( esp_wifi_connectionless_module_set_wake_interval(CONFIG_ESPNOW_WAKE_INTERVAL) );
#endif

    /* Set primary master key. */
    ESP_ERROR_CHECK( esp_now_set_pmk((uint8_t *)CONFIG_ESPNOW_PMK) ); 

    return ESP_OK;
}

void master_espnow_deinit()
{
    esp_now_deinit();
}

void master_espnow_protocol()
{
    // Initialize xFreeRTOS
    table_devices_mutex = xSemaphoreCreateMutex();
    xEventGroup = xEventGroupCreate();
    xEventGroupLightSleep = xEventGroupCreate();
    slave_disconnect_queue = xQueueCreate(10, sizeof(uint32_t));

    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) 
    {
        ESP_ERROR_CHECK( nvs_flash_erase() );
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK( ret );

    // Initialize wifi_espnow
    master_wifi_init();

    // Initialize temperature mcu esp
    init_temperature_sensor();

    /* ----------Data demo MAC from Slave---------- */
    test_allowed_connect_slaves_to_nvs(test_allowed_connect_slaves);
    save_info_slaves_to_nvs("KEY_SLA_ALLOW", test_allowed_connect_slaves);
    load_info_slaves_from_nvs("KEY_SLA_ALLOW", allowed_connect_slaves);
    /* End----------Data demo MAC from Slave---------- */

    // Load MAC and status Slave to Table Devices
    for (int i = 0; i < MAX_SLAVES; i++) 
    {
        write_table_devices(allowed_connect_slaves[i].peer_addr, NULL, allowed_connect_slaves[i].status);
    }

    // Initialize espnow
    master_espnow_init();

    xTaskCreate(master_espnow_task, "master_espnow_task", 4096, &send_param, 4, &master_espnow_handle);
    xTaskCreate(retry_connect_lost_task, "retry_connect_lost_task", 4096, NULL, 4, &retry_connect_lost_handle);
    xTaskCreate(light_sleep_task, "light_sleep_task", 4096, NULL, 3, NULL);
}