#include "uart.h"
#include "pub_sub_client.h"

uint8_t mac1[6] = {0xdc, 0xda, 0x0c, 0x0d, 0x42, 0x48}; //
uint8_t mac2[6] = {0xd8, 0x3b, 0xda, 0x9a, 0x34, 0xac}; //
uint8_t mac3[6] = {0xdc, 0xda, 0x0c, 0x18, 0xbc, 0x84}; //

static const char *TAG = "UART";
static uint32_t crc_error_count = 0;
int i=0;

device_t devices[MAX_DEVICES];
int num_devices = 0;
//sensor_data_t *recv_data;
sensor_data_t recv_data;
QueueHandle_t uart1_queue;

void process_uart_data(uint8_t *data, size_t length) {
    
    size_t expected_len = sizeof(sensor_data_uart_t);
    if (length < expected_len) {
        ESP_LOGE(TAG, "Data size is too small: %d bytes, expected %d bytes", length, expected_len);
        //return; /* */
    }
    sensor_data_uart_t recv_packet; 
    sensor_data_uart_t *recv_packet_ptr = &recv_packet;
    memcpy(recv_packet_ptr, data, expected_len);

    size_t data_len = expected_len - sizeof(recv_packet_ptr->crc);
    uint16_t calculated_crc = esp_crc16_le(UINT16_MAX, (const uint8_t *)recv_packet_ptr, data_len);
    ESP_LOGI(TAG, "message recv: %s",recv_packet_ptr->message);
    //printf("message recv: %s\n", recv_packet_ptr->message);
    if (recv_packet_ptr->crc == calculated_crc) {
        ESP_LOGI(TAG, "Received valid data");
        // Gán dữ liệu từ recv_packet_ptr vào cấu trúc sensor_data_t để xử lý
        recv_data = recv_packet_ptr->data;
        // Xử lý dữ liệu nhận được
        update_or_add_device(&recv_data);
        log_all_devices();
        //mqtt
        
    } else {
        ESP_LOGE(TAG, "CRC mismatch: Received CRC = 0x%04X, Calculated CRC = 0x%04X", recv_packet_ptr->crc, calculated_crc);
        crc_error_count++;
        uart_flush_input(UART1_NUM);
    }
    printf("Total CRC errors: %ld\n", crc_error_count);

}
void send_data_to_mqtt(int device_index, void (*data_to_mqtt_func)(const char*, const char*, int, int)) {
    char datatomqtt[100];   
    sprintf(datatomqtt, "temperature_rdo: %f, do: %f, temperature_phg: %f, ph: %f",
            devices[device_index].data.temperature_rdo,
            devices[device_index].data.do_value,
            devices[device_index].data.temperature_phg,
            devices[device_index].data.ph_value);
    
    data_to_mqtt_func(datatomqtt, "v1/devices/me/telemetry", 500, 1);
    vTaskDelete(NULL);
}

void datatomqtt1() {
    send_data_to_mqtt(0, data_to_mqtt);
}
void datatomqtt2() {
    send_data_to_mqtt(1, data_to_mqtt2);
}
int sendData(const char* logName, const char* data)
{
    const int len = strlen(data);
    const int txBytes = uart_write_bytes(UART1_NUM, data, len);
    ESP_LOGI(logName, "Wrote 1 %d bytes: %s", txBytes, data);
    return txBytes;
}
void tx1_task(void *arg)
{
    static const char *TX_TASK_TAG = "TX_TASK";
    esp_log_level_set(TX_TASK_TAG, ESP_LOG_INFO);
    while (1) {
        sendData(TX_TASK_TAG, "Hello world\n");
        vTaskDelay(10000 / portTICK_PERIOD_MS);
    }
}
void rx1_task(void *arg)
{
    static const char *RX1_TASK_TAG = "RX1_TASK";
    esp_log_level_set(RX1_TASK_TAG, ESP_LOG_INFO);
    uint8_t* data = (uint8_t*) malloc(BUF_SIZE + 1);
    while (1) {
        const int rxBytes = uart_read_bytes(UART1_NUM, data, BUF_SIZE, 1000 / portTICK_PERIOD_MS);
        if (rxBytes > 0) {
            data[rxBytes] = 0;
            ESP_LOGI(RX1_TASK_TAG, "Read %d bytes: '%s'", rxBytes, data);
            ESP_LOG_BUFFER_HEXDUMP(RX1_TASK_TAG, data, rxBytes, ESP_LOG_INFO);
            //parse_json((char *)data);
        }
    }
    free(data);
}
/*Check mac exist ??*/
int find_device_index(uint8_t mac[6]) {
    for (int i = 0; i < num_devices; i++) {
        if (memcmp(devices[i].mac, mac, 6) == 0) {
            return i;
        }
    }
    return -1;  // Không tìm thấy thiết bị
}
void update_or_add_device(sensor_data_t *new_data) {
    int index = find_device_index(new_data->mac);
    if (index != -1) {
        ESP_LOGI("DEVICE", "Update Device Data");
        devices[index].data = *new_data;
    } else {
        // Thêm thiết bị mới nếu còn chỗ
        if (num_devices < MAX_DEVICES) {
            memcpy(devices[num_devices].mac, new_data->mac, 6);
            devices[num_devices].data = *new_data;
            num_devices++;
            ESP_LOGI("DEVICE", "Add New Device");
        } else {
            // Xử lý khi danh sách đầy
            ESP_LOGW("DEVICE", "Device list is full, can't add new device");
        }
    }
}
void log_all_devices() { /*Log all device data*/
    ESP_LOGI("DEVICE", "Total devices: %d", num_devices);
    for (int i = 0; i < num_devices; i++) {
        //if(i==2) {datatomqtt2();}
        if(i==0) {
            xTaskCreate(datatomqtt1, "mqtt_task", 5000, NULL, 5, NULL);
            //datatomqtt2();
        }
        if(i==1) {
            xTaskCreate(datatomqtt2,  "mqtt_task", 5000, NULL, 5, NULL);
            //datatomqtt();
        }
        ESP_LOGE("DEVICE", "Device %d:", i + 1);
        ESP_LOGI("DEVICE", "          MAC Address: %02X:%02X:%02X:%02X:%02X:%02X",
                devices[i].mac[0], devices[i].mac[1], devices[i].mac[2],
                devices[i].mac[3], devices[i].mac[4], devices[i].mac[5]);
        ESP_LOGI("DEVICE", "          Temperature MCU: %.6f", devices[i].data.temperature_mcu);
        ESP_LOGI("DEVICE", "          RSSI: %d", devices[i].data.rssi);
        ESP_LOGI("DEVICE", "          Temperature RDO: %.6f", devices[i].data.temperature_rdo);
        ESP_LOGI("DEVICE", "          DO Value: %.6f", devices[i].data.do_value);
        ESP_LOGI("DEVICE", "          Temperature PHG: %.6f", devices[i].data.temperature_phg);
        ESP_LOGI("DEVICE", "          PH Value: %.6f", devices[i].data.ph_value);
        
    }
}
void uart_init(void){
    esp_log_level_set(TAG, ESP_LOG_INFO);
    /*Config Uart 1 - Uart 0 is default*/
    uart_config_t uart_config1 = {
        .baud_rate = UART_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    int intr_alloc_flags = 0;
    /*Config Uart 2*/
    /*Config Uart 1 - Uart 0 is default*/
    ESP_ERROR_CHECK(uart_driver_install(UART1_NUM, BUF_SIZE * 4, BUF_SIZE * 4, QUEUE_SIZE, &uart1_queue, 0)); //uart_driver_install(uart_port_t uart_num, int rx_buffer_size, int tx_buffer_size, int queue_size, QueueHandle_t *uart_queue, int intr_alloc_flags)
    ESP_ERROR_CHECK(uart_param_config(UART1_NUM, &uart_config1));
    esp_log_level_set(TAG, ESP_LOG_INFO);
    ESP_ERROR_CHECK(uart_set_pin(UART1_NUM, TXD1_PIN, RXD1_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE)); //uart_set_pin(uart_port_t uart_num, int tx_io_num, int rx_io_num, int rts_io_num, int cts_io_num)
    uart_enable_pattern_det_baud_intr(UART1_NUM, '+', PATTERN_CHR_NUM, 9, 0, 0);
    uart_pattern_queue_reset(UART1_NUM, QUEUE_SIZE);
}
void uart_event_task(void *pvParameters)
{
    uart_event_t event;
    size_t buffered_size;
    uint8_t* dtmp = (uint8_t*) malloc(RD_BUF_SIZE + 1);
    unsigned char recv_message[sizeof(sensor_data_t)];
    //sensor_data_t recv_data;
    if (dtmp == NULL) {
        ESP_LOGE(TAG, "Failed to allocate memory for dtmp");
        vTaskDelete(NULL);
    }
    while (true) {
        //Waiting for UART event.
        if (xQueueReceive(uart1_queue, (void *)&event, (TickType_t)portMAX_DELAY)) {
            memset(dtmp, 0, RD_BUF_SIZE);
            ESP_LOGE(TAG, "uart[%d] event", UART1_NUM);
            switch (event.type) {
            case UART_DATA:
                ESP_LOGI(TAG, "[UART DATA]: %d", event.size);
                int length = uart_read_bytes(UART1_NUM, recv_message, event.size, portMAX_DELAY);
                ESP_LOGE(TAG,"recv_message :::%s", recv_message);

                if (strcmp((char*)recv_message, "CONNECT") == 0) {
                    send_mac_via_uart(mac1, 6);
                    send_mac_via_uart(mac2, 6);
                    send_mac_via_uart(mac3, 6);
                }

                process_uart_data(recv_message, length);
                uart_write_bytes(UART1_NUM, (const char*) dtmp, event.size);
                /*---------------------------------------------------*/
                break;
            case UART_FIFO_OVF:
                ESP_LOGI(TAG, "hw fifo overflow");
                uart_flush_input(UART1_NUM);
                xQueueReset(uart1_queue);
                break;
            case UART_BUFFER_FULL:
                ESP_LOGI(TAG, "ring buffer full");
                uart_flush_input(UART1_NUM);
                xQueueReset(uart1_queue);
                break;
            case UART_BREAK:
                ESP_LOGW(TAG, "uart rx break");
                break;
            case UART_PARITY_ERR:
                ESP_LOGI(TAG, "uart parity error");
                break;
            case UART_FRAME_ERR:
                ESP_LOGI(TAG, "uart frame error");
                break;
            case UART_PATTERN_DET:
                uart_get_buffered_data_len(UART1_NUM, &buffered_size);
                int pos = uart_pattern_pop_pos(UART1_NUM);
                ESP_LOGI(TAG, "[UART PATTERN DETECTED] pos: %d, buffered size: %d", pos, buffered_size);
                if (pos == -1) {
                    uart_flush_input(UART1_NUM);
                } else {
                    uart_read_bytes(UART1_NUM, dtmp, pos, 100 / portTICK_PERIOD_MS);
                    uint8_t pat[PATTERN_CHR_NUM + 1];
                    memset(pat, 0, sizeof(pat));
                    uart_read_bytes(UART1_NUM, pat, PATTERN_CHR_NUM, 100 / portTICK_PERIOD_MS);
                    ESP_LOGI(TAG, "read data: %s", dtmp);
                    ESP_LOGI(TAG, "read pat : %s", pat);
                }
                break;
            //case UART_TAKE_OFF:
            default:
                ESP_LOGI(TAG, "uart event type: %d", event.type);
                break;
            }
        }
    }
    free(dtmp);
    dtmp = NULL;
    vTaskDelete(NULL);
}
void send_uart_(int UART_NUM, const char *_message) {
    int len = strlen(_message);
    
    // Gửi chuỗi ACK qua UART
    int written = uart_write_bytes(UART_NUM, _message, len);
    
    if (written == len) {
        ESP_LOGI("UART_ACK", "ACK sent successfully: %s", _message);
    } else {
        ESP_LOGE("UART_ACK", "Failed to send ACK: %s", _message);
    }
}
void send_mac_via_uart(uint8_t *mac_address, int mac_len) {

    char mac_string[18];
    snprintf(mac_string, sizeof(mac_string), 
             "%02x:%02x:%02x:%02x:%02x:%02x", 
             mac_address[0], mac_address[1], mac_address[2], 
             mac_address[3], mac_address[4], mac_address[5]);
    uart_write_bytes(UART1_NUM, "\n", 1);         
    uart_write_bytes(UART1_NUM, mac_string, strlen(mac_string));
    uart_write_bytes(UART1_NUM, "\n", 1);
}