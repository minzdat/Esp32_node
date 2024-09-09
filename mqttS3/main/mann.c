#include "driver/uart.h"
#include "esp_log.h"
#include "string.h"


typedef struct {
    uint32_t timestamp;         // 4 bytes - Thời gian ghi nhận dữ liệu
    float temperature;          // 4 bytes - Nhiệt độ
    float humidity;             // 4 bytes - Độ ẩm
    float pressure;             // 4 bytes - Áp suất khí quyển
    float co2_level;            // 4 bytes - Mức CO2
    float voc_level;            // 4 bytes - Mức VOC (hợp chất hữu cơ bay hơi)
    float pm2_5;                // 4 bytes - Nồng độ bụi mịn PM2.5
    float pm10;                 // 4 bytes - Nồng độ bụi PM10
    uint16_t wind_speed;        // 2 bytes - Tốc độ gió
    uint16_t wind_direction;    // 2 bytes - Hướng gió
    uint16_t rainfall;          // 2 bytes - Lượng mưa
    uint16_t light_intensity;   // 2 bytes - Cường độ ánh sáng
    uint16_t crc;
    uint8_t sensor_status[28];  // 30 bytes - Trạng thái các cảm biến
    char location[50];          // 50 bytes - Vị trí của trạm cảm biến

} env_sensor_data_t;

    env_sensor_data_t sensor_data = {
        .timestamp = 1627890123,
        .temperature = 25.5,
        .humidity = 60.2,
        .pressure = 1013.25,
        .co2_level = 400.5,
        .voc_level = 150.2,
        .pm2_5 = 35.7,
        .pm10 = 50.3,
        .wind_speed = 5,
        .wind_direction = 180,
        .rainfall = 12,
        .light_intensity = 800,
        .crc=0,
        .sensor_status = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE}, // Mẫu trạng thái cảm biến
        .location = "TP THU DUC, TP HO CHI MINH, VietNam"
    };

#define UART_NUM UART_NUM_1
#define TX_PIN 17
#define RX_PIN 16
#define BUF_SIZE 1024

void init_uart() {
    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    };
    uart_param_config(UART_NUM, &uart_config);
    uart_set_pin(UART_NUM, TX_PIN, RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    uart_driver_install(UART_NUM, BUF_SIZE, 0, 0, NULL, 0);
}

void print_sensor_data(const env_sensor_data_t *data) {
    ESP_LOGI("UART","Timestamp: %lu\n", data->timestamp);
    ESP_LOGI("UART","Temperature: %.2f°C\n", data->temperature);
    ESP_LOGI("UART","Humidity: %.2f%%\n", data->humidity);
    ESP_LOGI("UART","Pressure: %.2f hPa\n", data->pressure);
    ESP_LOGI("UART","CO2 Level: %.2f ppm\n", data->co2_level);
    ESP_LOGI("UART","VOC Level: %.2f ppb\n", data->voc_level);
    ESP_LOGI("UART","PM2.5: %.2f µg/m³\n", data->pm2_5);
    ESP_LOGI("UART","PM10: %.2f µg/m³\n", data->pm10);
    ESP_LOGI("UART","Wind Speed: %u m/s\n", data->wind_speed);
    ESP_LOGI("UART","Wind Direction: %u°\n", data->wind_direction);
    ESP_LOGI("UART","Rainfall: %u mm\n", data->rainfall);
    ESP_LOGI("UART","CRC16: %u \n", data->crc);
    ESP_LOGI("UART","Light Intensity: %u lux\n", data->light_intensity);
    ESP_LOGI("UART","Sensor Status: ");
    for (int i = 0; i < 30; ++i) {
        printf("%02X ", data->sensor_status[i]);
    }
    printf("\n");
    ESP_LOGI("UART","\nLocation: %s\n", data->location);
}


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
uint16_t calculate_checksum(uint8_t *data, size_t len) {
    uint16_t crc = 0xFFFF;
    for (size_t i = 0; i < len; i++) {
        crc ^= (uint16_t)data[i] << 8;
        for (uint8_t j = 0; j < 8; j++) {
            if (crc & 0x8000) {
                crc = (crc << 1) ^ 0x1021;
            } else {
                crc = crc << 1;
            }
        }
    }
    return crc;
}

int count;
// typedef struct {
//     uint8_t id;
//     float temperature;
//     float humidity;
//     uint16_t crc;
// } sensor_data_t;
    
// sensor_data_t sensor_dataa = {
//         .id = 1,
//         .temperature = 25.3,
//         .humidity = 60.5,
//         .crc=0
//     };


// uint8_t crc, crc_cal = 0;
bool uart_receive_struct_with_checksum(env_sensor_data_t* data) {
    size_t length = sizeof(env_sensor_data_t);
    uint8_t buffer[length + 1];
    uint16_t crc_d;
    // int len = uart_read_bytes(UART_NUM, buffer, length , 1000 / portTICK_PERIOD_MS);
    int len = uart_read_bytes(UART_NUM, buffer, length , portMAX_DELAY);

    // printf("Len receiv: ");
    // printf("%d \n",len);
        // uint8_t received_checksum = buffer[length];
    memcpy(data, buffer, length);
    crc_d=data->crc;
    data->crc=0;
    uint16_t calculated_checksum = calculate_checksum(data, length);
    // printf("CRC receiv: ");
    // printf("%u \n",calculated_checksum);
    // printf("crc_d %u \n",crc_d);
    
    if (crc_d == calculated_checksum) {
        return true;
        }
    else{
        count++;
        // printf("%d ",count);
        ESP_LOGE("error", " error: %d",count);

        }
    return false;
}
void uart_send_struct_with_checksum(const env_sensor_data_t* data) {
    size_t length = sizeof(env_sensor_data_t);
    // uint8_t checksum = calculate_checksum((const uint8_t*)data, length);
    // sensor_data_t* buff=(sensor_data_t*)data;
    // buff->crc=checksum;
    uart_write_bytes(UART_NUM, (const char*)data, length);
    // uart_write_bytes(UART_NUM, (const char*)&checksum, 1);
    
}


void sendd(){
    while (true)
    {
    uart_send_struct_with_checksum(&sensor_data);
        // uart_write_bytes(UART_NUM, (const char*)sensor_data, sizeof);
    vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}
int countt=0;
void receiv(){
    while (1) {
        env_sensor_data_t received_data;

        if (!uart_receive_struct_with_checksum(&received_data)) {
countt++;
    ESP_LOGE("UART", " ERROR: _______________________________________________\n%d",countt);
            ESP_LOGE("UART", "Checksum error:");
            print_hex(&received_data,120);

            ESP_LOGE("UART", "True buffer:");
            print_hex(&sensor_data,120);


uart_flush(UART_NUM);


            // ESP_LOGI("UART", "Received data - ID: %d, Temperature: %.2f, Humidity: %.2f, CRC: %d",
            //          received_data.id, received_data.temperature, received_data.humidity, received_data.crc);
                // print_sensor_data(&received_data);

    // ESP_LOGI("TAG", "  crc_buffer  : %d", received_data.crc);
    // // crc = received_data.crc;
    // // received_data.crc = 0;
    // // crc_cal = esp_crc16_le(UINT16_MAX, (uint8_t const *)buf, data_len);
    // crc_cal = calculate_checksum((uint8_t*)&received_data, sizeof(received_data));
    // //  calculate_checksum(buffer, length);

    // ESP_LOGI("TAG", "  crc_receiver: %d", crc_cal);
        // } else {
        //     ESP_LOGE("UART", "Checksum error");
        //     print_hex(&received_data,120);

        }

        // Thực hiện lặp lại sau một khoảng thời gian (nếu cần)
        // vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

uint16_t crc_r;
void app_main() {
    // Khởi tạo UART
    init_uart();
    print_sensor_data(&sensor_data);
    print_hex(&sensor_data,120);
    crc_r=calculate_checksum(&sensor_data,120);
    ESP_LOGI("UART","CRC16: %u\n", crc_r);

    sensor_data.crc=crc_r;

    print_sensor_data(&sensor_data);
    print_hex(&sensor_data,120);
    uint16_t crc=calculate_checksum(&sensor_data,120);
    ESP_LOGI("UART","CRC16: %u\n", crc);

    // Gửi struct kèm checksum
xTaskCreate(sendd, "send_uart", 5000, NULL, 5, NULL);
xTaskCreate(receiv, "receiv", 5000, NULL, 5, NULL);


    // Nhận và kiểm tra struct
    
}

