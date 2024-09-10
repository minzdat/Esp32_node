#include "uart.h"
static const char *TAG = "UART";
static uint32_t crc_error_count = 0;

device_t devices[MAX_DEVICES];
int num_devices = 0;

void process_uart_data(uint8_t *data, size_t length) {
    
    size_t data_len = sizeof(sensor_data_t);
    size_t crc_len = sizeof(uint16_t);
    if (length < (data_len + crc_len)) {
        ESP_LOGE(TAG, "Data size is too small: %d bytes", length);
        return;
    }
    sensor_data_t *recv_data = (sensor_data_t *)data;
    uint16_t received_crc = 0;
    memcpy(&received_crc, data + data_len, crc_len); // Sao chép CRC từ buffer

    // Tính toán CRC của dữ liệu nhận được
    uint16_t calculated_crc = esp_crc16_le(UINT16_MAX, data, data_len);

    // Kiểm tra CRC
    if (received_crc == calculated_crc) {
        // ESP_LOGI(TAG, "Received valid data");
        // ESP_LOGW(TAG, "      MAC " MACSTR " (length: %d): ", MAC2STR(recv_data->mac), length);
        // ESP_LOGI(TAG, "      Temperature MCU: %.6f", recv_data->temperature_mcu);
        // ESP_LOGI(TAG, "      RSSI: %d", recv_data->rssi);
        // ESP_LOGI(TAG, "      Temperature RDO: %.6f", recv_data->temperature_rdo);
        // ESP_LOGI(TAG, "      Dissolved Oxygen: %.6f", recv_data->do_value);
        // ESP_LOGI(TAG, "      Temperature PHG: %.6f", recv_data->temperature_phg);
        // ESP_LOGI(TAG, "      pH: %.6f", recv_data->ph_value);
        
        // Cập nhật hoặc thêm thiết bị mới vào danh sách
        update_or_add_device(recv_data);
        log_all_devices();
    } else {
        ESP_LOGE(TAG, "CRC mismatch: Received CRC = 0x%04X, Calculated CRC = 0x%04X", received_crc, calculated_crc);
        crc_error_count++;
        uart_flush_input(UART1_NUM);
    }
    printf("Total CRC errors: %ld\n", crc_error_count);
}



int sendData(const char* logName, const char* data)
{
    const int len = strlen(data);
    const int txBytes = uart_write_bytes(UART1_NUM, data, len);
    ESP_LOGI(logName, "Wrote 1 %d bytes: %s", txBytes, data);
    return txBytes;
}
static void tx1_task(void *arg)
{
    static const char *TX_TASK_TAG = "TX_TASK";
    esp_log_level_set(TX_TASK_TAG, ESP_LOG_INFO);
    while (1) {
        sendData(TX_TASK_TAG, "Hello world\n");
        vTaskDelay(10000 / portTICK_PERIOD_MS);
    }
}
static void rx1_task(void *arg)
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
