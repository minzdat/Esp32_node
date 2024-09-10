#include "master_espnow_protocol.h"
<<<<<<< HEAD

static int8_t rssi;
static char message_packed[MAX_PAYLOAD_LEN]; 
=======
#include "sleep.h"
#include "uart.h"

>>>>>>> 0d65c9acca272f1193113c0af4e2c3e13a3f601f
static int current_index = CURRENT_INDEX;
static const uint8_t s_master_broadcast_mac[ESP_NOW_ETH_ALEN] = MASTER_BROADCAST_MAC;
static QueueHandle_t s_master_espnow_queue;
static master_espnow_send_param_t send_param;
<<<<<<< HEAD
static master_espnow_send_param_t send_param_specified;
static uint16_t s_espnow_seq[ESPNOW_DATA_MAX] = { 0, 0 };
list_slaves_t test_allowed_connect_slaves[MAX_SLAVES];
list_slaves_t allowed_connect_slaves[MAX_SLAVES];
list_slaves_t waiting_connect_slaves[MAX_SLAVES];
sensor_data_t esp_data_sensor;
table_device_t table_devices[MAX_SLAVES];
SemaphoreHandle_t table_devices_mutex;

void log_table_devices() 
{
    ESP_LOGI(TAG, "---------------------------------------------------------------------------------------------------------");
        ESP_LOGI(TAG, "| %-17s | %-7s | %-7s | %-12s | %-12s | %-12s | %-8s | %-8s |", 
                 "MAC Address", "Status", "RSSI", "Temp MCU", "Temp RDO", "Temp PHG", "DO Value", "pH Value");
        ESP_LOGI(TAG, "--------------------------------------------------------------------------------------------------------");
        
        for (int i = 0; i < MAX_SLAVES; i++) 
        {
            if (memcmp(table_devices[i].peer_addr, "\0\0\0\0\0\0", ESP_NOW_ETH_ALEN) != 0) 
            {
                char mac_str[18];
                sprintf(mac_str, "%02X:%02X:%02X:%02X:%02X:%02X", 
                        table_devices[i].peer_addr[0], table_devices[i].peer_addr[1], table_devices[i].peer_addr[2],
                        table_devices[i].peer_addr[3], table_devices[i].peer_addr[4], table_devices[i].peer_addr[5]);

                ESP_LOGI(TAG, "| %-17s | %-7s | %-7d | %-12.2f | %-12.2f | %-12.2f | %-8.2f | %-8.2f |",
                         mac_str,
                         table_devices[i].status ? "Online" : "Offline",
                         table_devices[i].data.rssi,
                         table_devices[i].data.temperature_mcu,
                         table_devices[i].data.temperature_rdo,
                         table_devices[i].data.temperature_phg,
                         table_devices[i].data.do_value,
                         table_devices[i].data.ph_value);
            }
        }
 
    ESP_LOGI(TAG, "--------------------------------------------------------------------------------------------------------");
}

// The function stores the value into table_device_t
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

void prepare_payload(espnow_data_t *espnow_data, float temperature_mcu, int rssi, float temperature_rdo, float do_value, float temperature_phg, float ph_value) 
{
    // Initialize sensor data directly in the payload field
    espnow_data->payload.temperature_mcu = temperature_mcu;
    espnow_data->payload.rssi = rssi;
    espnow_data->payload.temperature_rdo = temperature_rdo;
    espnow_data->payload.do_value = do_value;
    espnow_data->payload.temperature_phg = temperature_phg;
    espnow_data->payload.ph_value = ph_value;

    // Print payload size and data for testing
    ESP_LOGI(TAG, "     Payload size: %d bytes", sizeof(sensor_data_t));
    ESP_LOGI(TAG, "         MCU Temperature: %.2f", espnow_data->payload.temperature_mcu);
    ESP_LOGI(TAG, "         RSSI: %d", espnow_data->payload.rssi);
    ESP_LOGI(TAG, "         RDO Temperature: %.2f", espnow_data->payload.temperature_rdo);
    ESP_LOGI(TAG, "         DO Value: %.2f", espnow_data->payload.do_value);
    ESP_LOGI(TAG, "         PHG Temperature: %.2f", espnow_data->payload.temperature_phg);
    ESP_LOGI(TAG, "         PH Value: %.2f", espnow_data->payload.ph_value);
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

    // Directly access the fields of the payload
    ESP_LOGI(TAG, "     Parsed ESPNOW payload:");
    ESP_LOGI(TAG, "         MCU Temperature: %.2f", espnow_data->payload.temperature_mcu);
    ESP_LOGI(TAG, "         RSSI: %d", espnow_data->payload.rssi);
    ESP_LOGI(TAG, "         RDO Temperature: %.2f", espnow_data->payload.temperature_rdo);
    ESP_LOGI(TAG, "         DO Value: %.2f", espnow_data->payload.do_value);
    ESP_LOGI(TAG, "         PHG Temperature: %.2f", espnow_data->payload.temperature_phg);
    ESP_LOGI(TAG, "         PH Value: %.2f", espnow_data->payload.ph_value);


        espnow_data_t sensor_data;
        espnow_data_t *buf = (espnow_data_t *)espnow_data;
        sensor_data_t bufff=buf->payload;
        // buf->crc = crc_cal;
        memcpy(&sensor_data,espnow_data, sizeof(espnow_data_t));
        // dump_uart((uint8_t*)buf, sizeof(espnow_data_t));
        dump_uart((uint8_t*)&sensor_data, 120);
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

    // Log the data received
    ESP_LOGI(TAG, "Parsed ESPNOW packed:");
    ESP_LOGI(TAG, "     type: %d", buf->type);
    ESP_LOGI(TAG, "     seq_num: %d", buf->seq_num);
    ESP_LOGI(TAG, "     crc: %d", buf->crc);
    ESP_LOGI(TAG, "     message: %s", buf->message);

    float temperature = read_internal_temperature_sensor();
    prepare_payload(buf, temperature, rssi, 0, 0, 0, 0);

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
=======
list_slaves_t test_allowed_connect_slaves[MAX_SLAVES];
list_slaves_t allowed_connect_slaves[MAX_SLAVES];
list_slaves_t waiting_connect_slaves[MAX_SLAVES];
>>>>>>> 0d65c9acca272f1193113c0af4e2c3e13a3f601f

void add_peer(const uint8_t *peer_mac, bool encrypt) 
{   
    esp_now_peer_info_t peer; 

    memset(&peer, 0, sizeof(esp_now_peer_info_t));
    peer.channel = CONFIG_ESPNOW_CHANNEL;
    peer.ifidx = ESPNOW_WIFI_IF;
<<<<<<< HEAD
    peer.encrypt = false;
=======
    peer.encrypt = encrypt;
>>>>>>> 0d65c9acca272f1193113c0af4e2c3e13a3f601f
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

<<<<<<< HEAD
=======

>>>>>>> 0d65c9acca272f1193113c0af4e2c3e13a3f601f
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
<<<<<<< HEAD
            ESP_LOGW(TAG, "Save WAITING_CONNECT_SLAVES_LIST into NVS");
            save_info_slaves_to_nvs("KEY_SLA_WAIT", waiting_connect_slaves); // Save to NVS

            // erase_key_in_nvs("KEY_SLA_ALLOW");
            // memset(allowed_connect_slaves, 0, sizeof(allowed_connect_slaves));

            // Callback function send WAITING_CONNECT_SLAVES_LIST to S3

=======
            ESP_LOGI(TAG, "Added MAC " MACSTR " to waiting allow connect list", MAC2STR(waiting_connect_slaves[i].peer_addr));
            ESP_LOGW(TAG, "Save waiting allow connect list into NVS");
            save_info_slaves_to_nvs("KEY_SLA_WAIT", waiting_connect_slaves); // Save to NVS
>>>>>>> 0d65c9acca272f1193113c0af4e2c3e13a3f601f
            return;
        }
    }
    
    // If the list is full, replace the MAC address at the current index in the ring list    
    memcpy(waiting_connect_slaves[current_index].peer_addr, mac_addr, ESP_NOW_ETH_ALEN);
<<<<<<< HEAD
    ESP_LOGW(TAG, "WAITING_CONNECT_SLAVES_LIST is full, replaced MAC at index %d with " MACSTR, current_index, MAC2STR(mac_addr));
=======
    ESP_LOGW(TAG, "Waiting allow connect list is full, replaced MAC at index %d with " MACSTR, current_index, MAC2STR(mac_addr));
>>>>>>> 0d65c9acca272f1193113c0af4e2c3e13a3f601f

    // Update index for next addition
    current_index = (current_index + 1) % MAX_SLAVES;

    // Save updated list to NVS
    save_info_slaves_to_nvs("KEY_SLA_WAIT", waiting_connect_slaves);
<<<<<<< HEAD

    // erase_key_in_nvs("KEY_SLA_ALLOW");
    // memset(allowed_connect_slaves, 0, sizeof(allowed_connect_slaves));

    // Callback function send WAITING_CONNECT_SLAVES_LIST to S3
=======
>>>>>>> 0d65c9acca272f1193113c0af4e2c3e13a3f601f
}

/* Function responds with the specified MAC and content*/
esp_err_t response_specified_mac(const uint8_t *dest_mac, const char *message, bool encrypt)
{
<<<<<<< HEAD
    send_param_specified.len = MAX_DATA_LEN;

    memcpy(send_param_specified.dest_mac, dest_mac, ESP_NOW_ETH_ALEN);

    espnow_data_prepare(&send_param_specified, message);

    add_peer(dest_mac, encrypt);

    // Send the unicast response
    if (esp_now_send(send_param_specified.dest_mac, send_param_specified.buffer, send_param_specified.len) != ESP_OK) 
    {
        ESP_LOGE(TAG, "Send error");
        master_espnow_deinit();
=======
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
>>>>>>> 0d65c9acca272f1193113c0af4e2c3e13a3f601f
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
<<<<<<< HEAD
    rssi = recv_info->rx_ctrl->rssi;
    // ESP_LOGI(TAG, "RSSI: %d dBm", rssi);

=======
>>>>>>> 0d65c9acca272f1193113c0af4e2c3e13a3f601f
    master_espnow_event_t evt;
    master_espnow_event_recv_cb_t *recv_cb = &evt.info.recv_cb;
    uint8_t * mac_addr = recv_info->src_addr;
    uint8_t * des_addr = recv_info->des_addr;
<<<<<<< HEAD

=======
    sensor_data_t *recv_data = (sensor_data_t *)data;
>>>>>>> 0d65c9acca272f1193113c0af4e2c3e13a3f601f
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
<<<<<<< HEAD
    {   
        ESP_LOGI(TAG, "_________________________________");
        ESP_LOGI(TAG, "Receive broadcast ESPNOW data");

=======
    {
        // Log the MAC address, destination address, and received data
        ESP_LOGI(TAG, "_________________________________");
        ESP_LOGI(TAG, "Receive broadcast ESPNOW data");
        ESP_LOGI(TAG, "Received data from MAC: " MACSTR ", Data Length: %d, Data: %.*s",
            MAC2STR(recv_cb->mac_addr), recv_cb->data_len, recv_cb->data_len, recv_cb->data);       
        
>>>>>>> 0d65c9acca272f1193113c0af4e2c3e13a3f601f
        bool found = false;
        // Check if the source MAC address is in the allowed slaves list
        for (int i = 0; i < MAX_SLAVES; i++) 
        {
            if (memcmp(allowed_connect_slaves[i].peer_addr, recv_cb->mac_addr, ESP_NOW_ETH_ALEN) == 0) 
            {
                if (!allowed_connect_slaves[i].status)  //Slave in status Offline
                {
<<<<<<< HEAD
                    // Parse the received data when the condition is met
                    espnow_data_parse(recv_cb->data, recv_cb->data_len);

                    if (recv_cb->data_len >= strlen(REQUEST_CONNECTION_MSG) && strstr((char *)message_packed, REQUEST_CONNECTION_MSG) != NULL) 
                    {
                        // Call a function to response agree connect
                        allowed_connect_slaves[i].start_time = esp_timer_get_time();
                        ESP_LOGW(TAG, "---------------------------------");
                        ESP_LOGW(TAG, "Response %s to MAC " MACSTR "",RESPONSE_AGREE_CONNECT,  MAC2STR(recv_cb->mac_addr));
                        response_specified_mac(recv_cb->mac_addr, RESPONSE_AGREE_CONNECT, false);
                    }        
=======
                    // Call a function to response agree connect
                    allowed_connect_slaves[i].start_time = esp_timer_get_time();
                    ESP_LOGW(TAG, "---------------------------------");
                    ESP_LOGW(TAG, "Response AGREE CONNECT to MAC " MACSTR "",  MAC2STR(recv_cb->mac_addr));
                    response_specified_mac(recv_cb->mac_addr, RESPONSE_AGREE_CONNECT, false);        
>>>>>>> 0d65c9acca272f1193113c0af4e2c3e13a3f601f
                }
                found = true;
                break;
            }
        }
        if (!found) 
        {
            // Call a function to add the slave to the waiting_connect_slaves list
<<<<<<< HEAD
            ESP_LOGW(TAG, "Add MAC " MACSTR " to WAITING_CONNECT_SLAVES_LIST",  MAC2STR(recv_cb->mac_addr));
=======
            ESP_LOGW(TAG, "Add MAC " MACSTR " to WAITING CONNECT SLAVES LIST",  MAC2STR(recv_cb->mac_addr));
>>>>>>> 0d65c9acca272f1193113c0af4e2c3e13a3f601f
            add_waiting_connect_slaves(recv_cb->mac_addr);
        }  
    } 
    else 
<<<<<<< HEAD
    {  
        ESP_LOGI(TAG, "_________________________________");
        ESP_LOGI(TAG, "Receive unicast ESPNOW data");







=======
    {
        ESP_LOGI(TAG, "_________________________________");
        ESP_LOGI(TAG, "Receive unicast ESPNOW data");
>>>>>>> 0d65c9acca272f1193113c0af4e2c3e13a3f601f
        // Check if the received data is SLAVE_SAVED_MAC_MSG to change status of MAC Online
        for (int i = 0; i < MAX_SLAVES; i++) 
        {
            // Update the status of the corresponding MAC address in allowed_connect_slaves list
            if (memcmp(allowed_connect_slaves[i].peer_addr, recv_cb->mac_addr, ESP_NOW_ETH_ALEN) == 0) 
            {
<<<<<<< HEAD
                // Parse the received data when the condition is met
                espnow_data_parse(recv_cb->data, recv_cb->data_len);

                if (recv_cb->data_len >= strlen(SLAVE_SAVED_MAC_MSG) && strstr((char *)message_packed, SLAVE_SAVED_MAC_MSG) != NULL) 
                {
                    allowed_connect_slaves[i].status = true; // Set status to Online

                    ESP_LOGI(TAG, "Updated MAC " MACSTR " status to %s", MAC2STR(recv_cb->mac_addr),  allowed_connect_slaves[i].status ? "online" : "offline");

                    allowed_connect_slaves[i].start_time = 0;
                    allowed_connect_slaves[i].number_retry = 0;

                    save_info_slaves_to_nvs("KEY_SLA_ALLOW", allowed_connect_slaves);
                    write_table_devices(allowed_connect_slaves[i].peer_addr, NULL, allowed_connect_slaves[i].status);

                    break;
                }
                else if (recv_cb->data_len >= strlen(STILL_CONNECTED_MSG) && strstr((char *)message_packed, STILL_CONNECTED_MSG) != NULL)
                {

                    write_table_devices(allowed_connect_slaves[i].peer_addr, &esp_data_sensor, allowed_connect_slaves[i].status);

                    allowed_connect_slaves[i].start_time = 0;
                    allowed_connect_slaves[i].check_connect_errors = 0;
                    // allowed_connect_slaves[i].count_receive++;

                    // ESP_LOGW(TAG, "Receive from " MACSTR " - Counting: %d", MAC2STR(allowed_connect_slaves[i].peer_addr), allowed_connect_slaves[i].count_receive);
                    
                    // vTaskDelay(pdMS_TO_TICKS(2000));

                    // deep_sleep_mode();

                    light_sleep_flag = true;
                    start_time_light_sleep = esp_timer_get_time();

                    break;
                }
=======
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
/*------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------*/
                    ESP_LOGI(TAG, "MAC " MACSTR " (length: %d): %.*s",MAC2STR(recv_cb->mac_addr), recv_cb->data_len, recv_cb->data_len, (char *)recv_cb->data);
                    ESP_LOGW(TAG, "Receive from " MACSTR " - Counting: %d", MAC2STR(allowed_connect_slaves[i].peer_addr), allowed_connect_slaves[i].count_receive);
                    break;
                }
                else{
                    allowed_connect_slaves[i].start_time = 0;
                    allowed_connect_slaves[i].check_connect_errors = 0;
                    allowed_connect_slaves[i].count_receive++;
                        // ESP_LOGW("","%s",(char *)recv_cb->data);
                        // recv_data->rssi=-5672367670;

            ESP_LOGW(TAG, "MAC " MACSTR " (length: %d): %.*s",MAC2STR(recv_data->mac), recv_cb->data_len, recv_cb->data_len, (char *)recv_data);
            ESP_LOGI(TAG, "Temperature MCU: %.6f", recv_data->temperature_mcu);
            ESP_LOGI("SENSOR_DATA", "RSSI: %d", recv_data->rssi);
            ESP_LOGI("SENSOR_DATA", "Temperature RDO: %.6f", recv_data->temperature_rdo);
            ESP_LOGI("SENSOR_DATA", "Dissolved Oxygen: %.6f", recv_data->do_value);
            ESP_LOGI("SENSOR_DATA", "Temperature PHG: %.6f", recv_data->temperature_phg);
            ESP_LOGI("SENSOR_DATA", "pH: %.6f", recv_data->ph_value);
                        // dump_uart((const char *)recv_cb->data);
                    send_uart(recv_data, "helzlo");

                    //sleep_init(, false, UART_NUM_1);
                    //go_to_sleep();
                    //sensor_data_t *send_uart = (sensor_data_t *)recv_cb; // Đây là struct bạn đã nhận được.
                    //sendStructData("TX1", recv_data, sizeof(recv_data));

                    //deep_sleep_mode();////
                    break;

                }
>>>>>>> 0d65c9acca272f1193113c0af4e2c3e13a3f601f
            }
        }
    }
}

void master_espnow_task(void *pvParameter)
{
    master_espnow_send_param_t *send_param = (master_espnow_send_param_t *)pvParameter;

<<<<<<< HEAD
    send_param->len = MAX_DATA_LEN;

    while (true) 
    {

        // ESP_LOGE(TAG, "Task master_espnow_task");
=======
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
>>>>>>> 0d65c9acca272f1193113c0af4e2c3e13a3f601f

        // Browse the allowed_connect_slaves list
        for (int i = 0; i < MAX_SLAVES; i++) 
        {
            if (allowed_connect_slaves[i].status) // Check online status
            { 
<<<<<<< HEAD
                // Update destination MAC address
                memcpy(send_param->dest_mac, allowed_connect_slaves[i].peer_addr, ESP_NOW_ETH_ALEN);
                ESP_LOGW(TAG, "---------------------------------");
                ESP_LOGW(TAG, "Send %s to MAC  " MACSTR "",CHECK_CONNECTION_MSG, MAC2STR(allowed_connect_slaves[i].peer_addr));

                // Count the number of packets sent
                // allowed_connect_slaves[i].count_send ++;
                // ESP_LOGW(TAG, "Send to " MACSTR " - Counting: %d", MAC2STR(allowed_connect_slaves[i].peer_addr), allowed_connect_slaves[i].count_send);
                                       
                espnow_data_prepare(send_param, CHECK_CONNECTION_MSG); 
         
                add_peer(allowed_connect_slaves[i].peer_addr, false);
=======
                add_peer(allowed_connect_slaves[i].peer_addr, true); 

                // Update destination MAC address
                memcpy(send_param->dest_mac, allowed_connect_slaves[i].peer_addr, ESP_NOW_ETH_ALEN);
                ESP_LOGW(TAG, "---------------------------------");
                ESP_LOGW(TAG, "Send to CHECK CONNECTION to MAC  " MACSTR "", MAC2STR(allowed_connect_slaves[i].peer_addr));

                // Count the number of packets sent
                allowed_connect_slaves[i].count_send ++;
                ESP_LOGW(TAG, "Send to " MACSTR " - Counting: %d", MAC2STR(allowed_connect_slaves[i].peer_addr), allowed_connect_slaves[i].count_send);
>>>>>>> 0d65c9acca272f1193113c0af4e2c3e13a3f601f

                if (esp_now_send(send_param->dest_mac, send_param->buffer, send_param->len) != ESP_OK) 
                {
                    allowed_connect_slaves[i].send_errors++;
                    ESP_LOGE(TAG, "Send error to MAC: " MACSTR ". Current send_errors count: %d", MAC2STR(send_param->dest_mac), allowed_connect_slaves[i].send_errors);
                    if (allowed_connect_slaves[i].send_errors >= MAX_SEND_ERRORS)
                    {
                        allowed_connect_slaves[i].status = false; // Mark MAC as offline
<<<<<<< HEAD

                        ESP_LOGW(TAG, "MAC: " MACSTR " has been marked offline", MAC2STR( allowed_connect_slaves[i].peer_addr));

                        // Reset value count if device marked offline
                        // allowed_connect_slaves[i].count_receive = 0;
                        // allowed_connect_slaves[i].count_send = 0;
                        // allowed_connect_slaves[i].count_retry = 0;

                        save_info_slaves_to_nvs("KEY_SLA_ALLOW", allowed_connect_slaves);
                        write_table_devices(allowed_connect_slaves[i].peer_addr, NULL, allowed_connect_slaves[i].status);
=======
                        ESP_LOGW(TAG, "MAC: " MACSTR " has been marked offline", MAC2STR( allowed_connect_slaves[i].peer_addr));

                        // Reset value count if device marked offline
                        allowed_connect_slaves[i].count_receive = 0;
                        allowed_connect_slaves[i].count_send = 0;
                        allowed_connect_slaves[i].count_retry = 0;
>>>>>>> 0d65c9acca272f1193113c0af4e2c3e13a3f601f
                    }
                } 
                else 
                {
                    allowed_connect_slaves[i].send_errors = 0;
                    allowed_connect_slaves[i].start_time = esp_timer_get_time(); 
                }
            }
        }
<<<<<<< HEAD

=======
>>>>>>> 0d65c9acca272f1193113c0af4e2c3e13a3f601f
        vTaskDelay(pdMS_TO_TICKS(TIME_CHECK_CONNECT));
    }
}

// Task Check the response from the Slaves
void retry_connect_lost_task(void *pvParameter)
{
    while (true) 
    {
<<<<<<< HEAD
        // ESP_LOGE(TAG, "Task retry_connect_lost_task");

=======
>>>>>>> 0d65c9acca272f1193113c0af4e2c3e13a3f601f
        for (int i = 0; i < MAX_SLAVES; i++) 
        {
            switch (allowed_connect_slaves[i].status) 
            {
                case false: // Slave Offline
                    if(allowed_connect_slaves[i].start_time > 0)
                    {
                        allowed_connect_slaves[i].end_time =  esp_timer_get_time();
                        uint64_t elapsed_time = allowed_connect_slaves[i].end_time - allowed_connect_slaves[i].start_time;

<<<<<<< HEAD
                        if (elapsed_time > RETRY_TIMEOUT) 
                        {
                            if (allowed_connect_slaves[i].number_retry >= NUMBER_RETRY)
                            {
                                ESP_LOGW(TAG, "---------------------------------");
                                ESP_LOGW(TAG, "Retry %s with " MACSTR "",RESPONSE_AGREE_CONNECT, MAC2STR(allowed_connect_slaves[i].peer_addr));
=======
                        if (elapsed_time > 8 * 1000000) 
                        {
                            if (allowed_connect_slaves[i].number_retry == NUMBER_RETRY)
                            {
                                ESP_LOGW(TAG, "---------------------------------");
                                ESP_LOGW(TAG, "Retry AGREE CONNECT with " MACSTR "", MAC2STR(allowed_connect_slaves[i].peer_addr));
>>>>>>> 0d65c9acca272f1193113c0af4e2c3e13a3f601f
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

<<<<<<< HEAD
                        if (elapsed_time > RETRY_TIMEOUT) 
                        {
                            allowed_connect_slaves[i].check_connect_errors++;
                            // allowed_connect_slaves[i].count_retry++;

                            ESP_LOGW(TAG, "---------------------------------");
                            // ESP_LOGW(TAG, "Retry %s with " MACSTR " Current count retry: %d",CHECK_CONNECTION_MSG, MAC2STR(allowed_connect_slaves[i].peer_addr), allowed_connect_slaves[i].count_retry);               
                            response_specified_mac(allowed_connect_slaves[i].peer_addr, CHECK_CONNECTION_MSG, true);    

                            if (allowed_connect_slaves[i].check_connect_errors >= NUMBER_RETRY)
                            {
                                allowed_connect_slaves[i].status = false;

=======
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
>>>>>>> 0d65c9acca272f1193113c0af4e2c3e13a3f601f
                                ESP_LOGW(TAG, "Change " MACSTR " has been marked offline", MAC2STR(allowed_connect_slaves[i].peer_addr));

                                allowed_connect_slaves[i].start_time = 0;
                                allowed_connect_slaves[i].check_connect_errors = 0;
<<<<<<< HEAD

                                save_info_slaves_to_nvs("KEY_SLA_ALLOW", allowed_connect_slaves);
                                write_table_devices(allowed_connect_slaves[i].peer_addr, NULL, allowed_connect_slaves[i].status);
=======
>>>>>>> 0d65c9acca272f1193113c0af4e2c3e13a3f601f
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
<<<<<<< HEAD

    /* Set primary master key. */
    ESP_ERROR_CHECK( esp_now_set_pmk((uint8_t *)CONFIG_ESPNOW_PMK) ); 

=======
    /* Set primary master key. */
    ESP_ERROR_CHECK( esp_now_set_pmk((uint8_t *)CONFIG_ESPNOW_PMK) ); 
    
    /* Initialize sending parameters. */
    memset(&send_param, 0, sizeof(master_espnow_send_param_t));
>>>>>>> 0d65c9acca272f1193113c0af4e2c3e13a3f601f

    return ESP_OK;
}

<<<<<<< HEAD
void master_espnow_deinit()
{
=======
void master_espnow_deinit(master_espnow_send_param_t *send_param)
{
    free(send_param->buffer);
    free(send_param);
>>>>>>> 0d65c9acca272f1193113c0af4e2c3e13a3f601f
    vSemaphoreDelete(s_master_espnow_queue);
    esp_now_deinit();
}

void master_espnow_protocol()
{
<<<<<<< HEAD
    table_devices_mutex = xSemaphoreCreateMutex();

=======
>>>>>>> 0d65c9acca272f1193113c0af4e2c3e13a3f601f
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

<<<<<<< HEAD
    for (int i = 0; i < MAX_SLAVES; i++) 
    {
        write_table_devices(allowed_connect_slaves[i].peer_addr, NULL, allowed_connect_slaves[i].status);
    }

    // ---Data demo MAC from Slave---

    // deep_sleep_register_rtc_timer_wakeup();
    // deep_sleep_register_gpio_wakeup();

    /* Enable wakeup from light sleep by timer */
    register_timer_wakeup();
    /* Enable wakeup from light sleep by gpio */
    register_gpio_wakeup();

    master_espnow_init();

    /* Initialize sending parameters. */
    memset(&send_param, 0, sizeof(master_espnow_send_param_t));

    xTaskCreate(master_espnow_task, "master_espnow_task", 4096, &send_param, 4, NULL);

    xTaskCreate(retry_connect_lost_task, "retry_connect_lost_task", 4096, NULL, 5, NULL);

    xTaskCreate(light_sleep_task, "light_sleep_task", 4096, NULL, 6, NULL);
}
=======
    // ---Data demo MAC from Slave---

    master_espnow_init();

    xTaskCreate(master_espnow_task, "master_espnow_task", 4096, &send_param, 4, NULL);

    xTaskCreate(retry_connect_lost_task, "retry_connect_lost_task", 4096, NULL, 5, NULL);
}
>>>>>>> 0d65c9acca272f1193113c0af4e2c3e13a3f601f
