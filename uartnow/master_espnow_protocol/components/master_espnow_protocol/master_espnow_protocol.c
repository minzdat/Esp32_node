#include "master_espnow_protocol.h"

static char payload[MAX_PAYLOAD_LEN]; 
static int current_index = CURRENT_INDEX;
static const uint8_t s_master_broadcast_mac[ESP_NOW_ETH_ALEN] = MASTER_BROADCAST_MAC;
static QueueHandle_t s_master_espnow_queue;
static master_espnow_send_param_t send_param;
static master_espnow_send_param_t send_param_specified;
static uint16_t s_espnow_seq[ESPNOW_DATA_MAX] = { 0, 0 };
list_slaves_t test_allowed_connect_slaves[MAX_SLAVES];
list_slaves_t allowed_connect_slaves[MAX_SLAVES];
list_slaves_t waiting_connect_slaves[MAX_SLAVES];

void prepare_payload(espnow_data_t *espnow_data, float temperature_mcu, int rssi, float temperature_rdo, float do_value, float temperature_phg, float ph_value, const char *message) 
{

    // Initialize variable sensor_data_t with input parameters
    sensor_data_t sensor_data = 
    {
        .temperature_mcu = temperature_mcu,
        .rssi = rssi,
        .temperature_rdo = temperature_rdo,
        .do_value = do_value,
        .temperature_phg = temperature_phg,
        .ph_value = ph_value
    };

    // Calculate the size of the message
    size_t message_size = strlen(message);

    // Copy message to sensor_data, ensuring not to exceed the allocated size
    strncpy(sensor_data.message, message, message_size);

    // Calculate the size of sensor_data_t
    size_t sensor_data_size = sizeof(sensor_data_t);

    // Make sure that the sensor_data_t size is not larger than the fixed size of the payload
    if (sensor_data_size > MAX_PAYLOAD_LEN) 
    {
        ESP_LOGE(TAG, "The sensor_data_t data is too large to fit in the payload");
        return;
    }

    // Copy sensor_data_t into the payload and fill in the rest as needed
    memcpy(espnow_data->payload, &sensor_data, sensor_data_size);

    // If the data is less than 120 bytes, fill the remainder with 0
    if (sensor_data_size < MAX_PAYLOAD_LEN) 
    {
        memset(espnow_data->payload + sensor_data_size, 0, MAX_PAYLOAD_LEN - sensor_data_size);
    }

    // Print payload size and data for testing
    ESP_LOGI(TAG, "     Payload size: %d bytes", MAX_PAYLOAD_LEN);
    ESP_LOGI(TAG, "         MCU Temperature: %.2f", sensor_data.temperature_mcu);
    ESP_LOGI(TAG, "         RSSI: %d", sensor_data.rssi);
    ESP_LOGI(TAG, "         RDO Temperature: %.2f", sensor_data.temperature_rdo);
    ESP_LOGI(TAG, "         DO Value: %.2f", sensor_data.do_value);
    ESP_LOGI(TAG, "         PHG Temperature: %.2f", sensor_data.temperature_phg);
    ESP_LOGI(TAG, "         PH Value: %.2f", sensor_data.ph_value);
    ESP_LOGI(TAG, "         Message: %s", sensor_data.message);

}

/* Parse ESPNOW data payload. */
void parse_payload(const espnow_data_t *espnow_data) 
{
    if (sizeof(espnow_data->payload) < sizeof(sensor_data_t)) 
    {
        ESP_LOGE(TAG, "Payload size is too small to parse sensor_data_t");
        return;
    }

    sensor_data_t sensor_data;
    memcpy(&sensor_data, espnow_data->payload, sizeof(sensor_data_t));

    ESP_LOGI(TAG, "     Parsed ESPNOW payload:");
    ESP_LOGI(TAG, "         MCU Temperature: %.2f", sensor_data.temperature_mcu);
    ESP_LOGI(TAG, "         RSSI: %d", sensor_data.rssi);
    ESP_LOGI(TAG, "         RDO Temperature: %.2f", sensor_data.temperature_rdo);
    ESP_LOGI(TAG, "         DO Value: %.2f", sensor_data.do_value);
    ESP_LOGI(TAG, "         PHG Temperature: %.2f", sensor_data.temperature_phg);
    ESP_LOGI(TAG, "         PH Value: %.2f", sensor_data.ph_value);
    ESP_LOGI(TAG, "         Message: %s", sensor_data.message);

    memcpy(payload, sensor_data.message, strlen(sensor_data.message) + 1);
}

/* Prepare ESPNOW data to be sent. */
void espnow_data_prepare(master_espnow_send_param_t *send_param, const char *message)
{
    espnow_data_t *buf = (espnow_data_t *)send_param->buffer;

    assert(send_param->len >= sizeof(espnow_data_t));

    buf->type = IS_BROADCAST_ADDR(send_param->dest_mac) ? ESPNOW_DATA_BROADCAST : ESPNOW_DATA_UNICAST;
    buf->seq_num = s_espnow_seq[buf->type]++;
    buf->crc = 0;

    float temperature = read_internal_temperature_sensor();
    prepare_payload(buf, temperature, -45, 23.1, 7.6, 24.0, 7.2, message);

    buf->crc = esp_crc16_le(UINT16_MAX, (uint8_t const *)buf, send_param->len);
}

/* Parse received ESPNOW data. */
void print_hex(const void* data, size_t len){
const uint8_t *byte_data = (const uint8_t*)data;
    // ESP_LOGI("UART","Hex Sensor: ");
    for (int i = 0; i < len; i++) {
        printf("0x%02X ", byte_data[i]);
        if ((i + 1) % 20 == 0) {
            printf("\n");  // Chia dòng sau mỗi 16 bytes cho dễ nhìn
        }
    }
    printf("\n");
}
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
    print_hex(buf, sizeof(data));

    crc = buf->crc;
    buf->crc = 0;
    crc_cal = esp_crc16_le(UINT16_MAX, (uint8_t const *)buf, data_len);
    ESP_LOGI(TAG, " crc_cal: %d", crc_cal);

    if (crc_cal == crc) 
    {
        ESP_LOGI(TAG, "CRC check passed.");

    } 
    else 
    {
        ESP_LOGE(TAG, "CRC check failed. Calculated CRC: %d, Received CRC: %d", crc_cal, crc);
        return;
    }

        // Log the payload if present
    if (data_len > sizeof(espnow_data_t)) 
    {
        parse_payload(buf);
        sensor_data_t sensor_data;
        // buf->crc = crc_cal;
        memcpy(&sensor_data, buf->payload, sizeof(sensor_data_t));
        // dump_uart((uint8_t*)buf, sizeof(espnow_data_t));
        dump_uart((uint8_t*)&sensor_data, sizeof(sensor_data_t));


    } 
    else 
    {
        ESP_LOGI(TAG, "  No payload data.");
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
}

/* Function responds with the specified MAC and content*/
esp_err_t response_specified_mac(const uint8_t *dest_mac, const char *message, bool encrypt)
{
    send_param_specified.len = MAX_DATA_LEN;

    memcpy(send_param_specified.dest_mac, dest_mac, ESP_NOW_ETH_ALEN);

    espnow_data_prepare(&send_param_specified, message);

    add_peer(dest_mac, encrypt);

    // Send the unicast response
    if (esp_now_send(send_param_specified.dest_mac, send_param_specified.buffer, send_param_specified.len) != ESP_OK) 
    {
        ESP_LOGE(TAG, "Send error");
        master_espnow_deinit(&send_param_specified);
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

                    if (recv_cb->data_len >= strlen(REQUEST_CONNECTION_MSG) && strstr((char *)payload, REQUEST_CONNECTION_MSG) != NULL) 
                    {
                        // Call a function to response agree connect
                        allowed_connect_slaves[i].start_time = esp_timer_get_time();
                        ESP_LOGW(TAG, "---------------------------------");
                        ESP_LOGW(TAG, "Response %s to MAC " MACSTR "",RESPONSE_AGREE_CONNECT,  MAC2STR(recv_cb->mac_addr));
                        response_specified_mac(recv_cb->mac_addr, RESPONSE_AGREE_CONNECT, false);
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

        // Check if the received data is SLAVE_SAVED_MAC_MSG to change status of MAC Online
        for (int i = 0; i < MAX_SLAVES; i++) 
        {
            // Update the status of the corresponding MAC address in allowed_connect_slaves list
            if (memcmp(allowed_connect_slaves[i].peer_addr, recv_cb->mac_addr, ESP_NOW_ETH_ALEN) == 0) 
            {
                // Parse the received data when the condition is met
                espnow_data_parse(recv_cb->data, recv_cb->data_len);

                if (recv_cb->data_len >= strlen(SLAVE_SAVED_MAC_MSG) && strstr((char *)payload, SLAVE_SAVED_MAC_MSG) != NULL) 
                {
                    allowed_connect_slaves[i].status = true; // Set status to Online
                    ESP_LOGI(TAG, "Updated MAC " MACSTR " status to %s", MAC2STR(recv_cb->mac_addr),  allowed_connect_slaves[i].status ? "online" : "offline");

                    allowed_connect_slaves[i].start_time = 0;
                    allowed_connect_slaves[i].number_retry = 0;

                    break;
                }
                else if (recv_cb->data_len >= strlen(STILL_CONNECTED_MSG) && strstr((char *)payload, STILL_CONNECTED_MSG) != NULL)
                {
                    allowed_connect_slaves[i].start_time = 0;
                    allowed_connect_slaves[i].check_connect_errors = 0;
                    allowed_connect_slaves[i].count_receive++;

                    ESP_LOGW(TAG, "Receive from " MACSTR " - Counting: %d", MAC2STR(allowed_connect_slaves[i].peer_addr), allowed_connect_slaves[i].count_receive);

                    break;
                }
            }
        }
    }
}

void master_espnow_task(void *pvParameter)
{
    master_espnow_send_param_t *send_param = (master_espnow_send_param_t *)pvParameter;

    send_param->len = MAX_DATA_LEN;

    while (true) 
    {

        ESP_LOGE(TAG, "Task master_espnow_task");

        // Browse the allowed_connect_slaves list
        for (int i = 0; i < MAX_SLAVES; i++) 
        {
            if (allowed_connect_slaves[i].status) // Check online status
            { 
                // Update destination MAC address
                memcpy(send_param->dest_mac, allowed_connect_slaves[i].peer_addr, ESP_NOW_ETH_ALEN);
                ESP_LOGW(TAG, "---------------------------------");
                ESP_LOGW(TAG, "Send %s to MAC  " MACSTR "",CHECK_CONNECTION_MSG, MAC2STR(allowed_connect_slaves[i].peer_addr));

                // Count the number of packets sent
                allowed_connect_slaves[i].count_send ++;
                ESP_LOGW(TAG, "Send to " MACSTR " - Counting: %d", MAC2STR(allowed_connect_slaves[i].peer_addr), allowed_connect_slaves[i].count_send);
                                       
                espnow_data_prepare(send_param, CHECK_CONNECTION_MSG); 
         
                add_peer(allowed_connect_slaves[i].peer_addr, true);

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

        // esp_deep_sleep_start();

        vTaskDelay(pdMS_TO_TICKS(TIME_CHECK_CONNECT));
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
                            if (allowed_connect_slaves[i].number_retry == NUMBER_RETRY)
                            {
                                ESP_LOGW(TAG, "---------------------------------");
                                ESP_LOGW(TAG, "Retry %s with " MACSTR "",RESPONSE_AGREE_CONNECT, MAC2STR(allowed_connect_slaves[i].peer_addr));
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

                        if (elapsed_time > RETRY_TIMEOUT) 
                        {
                            allowed_connect_slaves[i].check_connect_errors++;
                            allowed_connect_slaves[i].count_retry++;

                            ESP_LOGW(TAG, "---------------------------------");
                            ESP_LOGW(TAG, "Retry %s with " MACSTR " Current count retry: %d",CHECK_CONNECTION_MSG, MAC2STR(allowed_connect_slaves[i].peer_addr), allowed_connect_slaves[i].count_retry);               
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

    esp_sleep_enable_timer_wakeup(ENABLE_TIMER_WAKEUP);

    /* Set primary master key. */
    ESP_ERROR_CHECK( esp_now_set_pmk((uint8_t *)CONFIG_ESPNOW_PMK) ); 
    
    /* Initialize sending parameters. */
    memset(&send_param, 0, sizeof(master_espnow_send_param_t));

    return ESP_OK;
}

void master_espnow_deinit(master_espnow_send_param_t *send_param)
{
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

    // xTaskCreate(retry_connect_lost_task, "retry_connect_lost_task", 4096, NULL, 5, NULL);
}