#include "read_serial.h"
#include "pub_sub_client.h"

const char *TAG="Read Serial";
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "freertos/queue.h"
#include "driver/uart.h"
#include "driver/gpio.h"

#include "esp_timer.h"
#include "esp_crc.h"
#include "cJSON.h"

int time_now=0;
int time_check=0;
int timeout=10000000;
bool connect_check=true;


#define PATTERN_CHR_NUM    (3)         /*!< Set the number of consecutive and identical characters received by receiver which defines a UART pattern*/
#define UART_NUM         UART_NUM_1     // Sử dụng UART1
#define TX_GPIO_NUM     17    // Chân TX (thay đổi nếu cần)
#define RX_GPIO_NUM      16    // Chân RX (thay đổi nếu cần)
#define BAUD_RATE        115200         // Tốc độ baud
#define BUF_SIZE (5000)
#define RD_BUF_SIZE (BUF_SIZE)
static QueueHandle_t uart0_queue;

void uart_config(void){
        uart_config_t uart_config = {
        .baud_rate = BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    uart_driver_install(UART_NUM, BUF_SIZE, BUF_SIZE, 10, &uart0_queue, 0);

    // uart_driver_install(UART_NUM, BUF_SIZE * 2, BUF_SIZE * 2, 20, &uart0_queue, 0);
    uart_param_config(UART_NUM, &uart_config);
    // uart_set_pin(UART_NUM, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    uart_set_pin(UART_NUM, TX_GPIO_NUM, RX_GPIO_NUM, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    // uart0_queue = xQueueCreate(10, BUF_SIZE);
    // uart_set_pin(EX_UART_NUM, TX_PIN, RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
}

#include "mbedtls/aes.h"
#define BUF_SIZEz 1024

void encrypt_message(const unsigned char *input, unsigned char *output, size_t length) {
    mbedtls_aes_context aes;
    unsigned char key[16] = "7832477891326794";
    unsigned char iv[16] =  "4892137489723148";
    
    mbedtls_aes_init(&aes);
    mbedtls_aes_setkey_enc(&aes, key, 128);
    mbedtls_aes_crypt_cbc(&aes, MBEDTLS_AES_ENCRYPT, length, iv, input, output);
    mbedtls_aes_free(&aes);
}
void decrypt_message(const unsigned char *input, unsigned char *output, size_t length) {
    mbedtls_aes_context aes;
    unsigned char key[16] = "7832477891326794";  // Khóa bí mật (same as used for encryption)
    unsigned char iv[16] =  "4892137489723148";    // Vector khởi tạo (same as used for encryption)
    mbedtls_aes_init(&aes);
    mbedtls_aes_setkey_dec(&aes, key, 128);  // Thiết lập khóa giải mã
    mbedtls_aes_crypt_cbc(&aes, MBEDTLS_AES_DECRYPT, length, iv, input, output); // Giải mã
    mbedtls_aes_free(&aes);
}
void dump_uart(uint8_t *message, size_t len){
    
    // len = sizeof(len);
    printf("send \n");
    uint8_t encrypted_message[len]; // AES block size = 16 bytes
    // Mã hóa tin nhắn
    encrypt_message((unsigned char *)message, encrypted_message, len);
    // uart_write_bytes(UART_NUM_P2, (const char *)message, sizeof(sensor_data_t));
    uart_write_bytes(UART_NUM, (unsigned char *)message, len);
}
void add_json(){
    cJSON *json_mac = cJSON_CreateObject();
    cJSON *json_data = cJSON_CreateObject();


    // Thêm các phần tử vào JSON object
    cJSON_AddStringToObject(json_mac, "Mac", "AA:BD:CC:FF:DF:FE");
    cJSON_AddNumberToObject(json_data, "temp_mcu", 20.50);
    cJSON_AddNumberToObject(json_data, "temp_do", 30.40);
    cJSON_AddItemToObject(json_mac,"Data",json_data);

    // cJSON_AddStringToObject(json_mac, "Mac", "AK:BD:CC:AB:DF:FB");
    // cJSON_AddNumberToObject(json_data, "temp_mcu", 50.20);
    // cJSON_AddNumberToObject(json_data, "temp_do", 1.50);
    // cJSON_AddItemToObject(json_mac,"Data",json_data);
    // Chuyển đổi JSON object thành chuỗi JSON
    char *json_string = cJSON_Print(json_mac);
    // Xuất ra JSON string (in ra serial)
    // printf("Size: %d \n",strlen(json_string));
    // printf("JSON: %s\n", json_string);
    uart_write_bytes(UART_NUM,json_string, strlen(json_string));


}

#include "mbedtls/aes.h"
#define BUF_SIZEz (1024)



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



#define MAC2STR(a) (a)[0], (a)[1], (a)[2], (a)[3], (a)[4], (a)[5]
#define MACSTR "%02x:%02x:%02x:%02x:%02x:%02x"

void delay(int x){
    vTaskDelay(pdMS_TO_TICKS(x));
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

sensor_data_t sensor_data;

int get_data(float *data1, float *data2, float *data3, float *data4){
    // memcpy(data, &sensor_data, sizeof(sensor_data_t));
    // return sizeof(data);
    *data1=sensor_data.temperature_rdo;
    *data2=sensor_data.do_value;
    *data3=sensor_data.temperature_phg;
    *data4=sensor_data.ph_value;
    return 1;
}

extern QueueHandle_t g_mqtt_queue;

void parse_payload(const sensor_data_t *espnow_data) 
{
    if (sizeof(espnow_data) < sizeof(sensor_data_t)) 
    {
        ESP_LOGE(TAG, "Payload size is too small to parse sensor_data_t");
        // return;
    }

    
    memcpy(&sensor_data, espnow_data, sizeof(sensor_data_t));

    xQueueSend(g_mqtt_queue, &sensor_data, portMAX_DELAY);
    
    ESP_LOGI(TAG, "     Parsed ESPNOW payload:");
    ESP_LOGI(TAG, "         MCU Temperature: %.2f", sensor_data.temperature_mcu);
    ESP_LOGI(TAG, "         RSSI: %d", sensor_data.rssi);
    ESP_LOGI(TAG, "         RDO Temperature: %.2f", sensor_data.temperature_rdo);
    ESP_LOGI(TAG, "         DO Value: %.2f", sensor_data.do_value);
    ESP_LOGI(TAG, "         PHG Temperature: %.2f", sensor_data.temperature_phg);
    ESP_LOGI(TAG, "         PH Value: %.2f", sensor_data.ph_value);
    // ESP_LOGI(TAG, "         Message: %s", sensor_data.message);
    // if (strcmp((char *)sensor_data.message, REQUEST_CONNECTION_MSG) == 0) {
    //     connect_request keep_connect;
    //     memcpy(keep_connect.message,REQUEST_CONNECTION_MSG,sizeof(REQUEST_CONNECTION_MSG));
    //     // keep_connect.message
    //     // dump_uart(keep_connect,sizeof(keep_connect));
        
    //     dump_uart((const char *)keep_connect.message,  sizeof(keep_connect.message));

    //     time_now=esp_timer_get_time(); 
    //     conn=true;
    // }
    // memcpy(payload, sensor_data.message, strlen(sensor_data.message) + 1);
}

#define GPIO_OUTPUT_PIN 17

void configure_gpio_output() {
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << GPIO_OUTPUT_PIN),  // Select the GPIO pin
        .mode = GPIO_MODE_OUTPUT,                    // Set as output mode
        .pull_up_en = GPIO_PULLUP_DISABLE,           // Disable pull-up
        .pull_down_en = GPIO_PULLDOWN_DISABLE,       // Disable pull-down
        .intr_type = GPIO_INTR_DISABLE               // Disable interrupts
    };
    gpio_config(&io_conf);
}

void check_timeout(){
    while (1)
    {

        if ((time_now!=0)&&(connect_check))
        {
        time_check=esp_timer_get_time();
                printf("timenow: %d \n",time_now);
                printf("time_check: %d \n",time_check);
        if ((time_check-time_now)>timeout){
            ESP_LOGE(TAG, "TIMEOUT UART");
            connect_check=false;
            time_now=0;
    //         wait_connect_serial();
    //         // break;
        }
        }
        delay(1000);

    }
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
void parse_payloadd(const espnow_data_t *espnow_data) {

    espnow_data_t *buf = (espnow_data_t *)espnow_data;
   ESP_LOGI(TAG, "  type   : %d", buf->type);
    ESP_LOGI(TAG, "  seq_num: %d", buf->seq_num);


    uint16_t crc, crc_cal = 0;
    ESP_LOGI(TAG, "  crc_buffer  : %d", buf->crc);
    crc = buf->crc;
    buf->crc = 0;
    crc_cal = esp_crc16_le(UINT16_MAX, (uint8_t const *)buf, 250);

    ESP_LOGI(TAG, "  crc_receiver: %d", crc_cal);


    if (sizeof(buf->payload) < sizeof(sensor_data_t)) 
    {
        ESP_LOGE(TAG, "Payload size is too small to parse sensor_data_t");
        return;
    }

    sensor_data_t sensor_data;
    memcpy(&sensor_data,&buf->payload, sizeof(sensor_data_t));

    // ESP_LOGW(TAG, "MAC " MACSTR " (length: %d): %.*s",MAC2STR(sensor_data.mac), recv_cb->data_len, recv_cb->data_len, (char *)sensor_data);

    ESP_LOGI(TAG, "Parsed Sensor Data:");

 
    ESP_LOGI(TAG, "     MCU Temperature: %.2f", sensor_data.temperature_mcu);
    ESP_LOGI(TAG, "     RSSI: %d", sensor_data.rssi);
    ESP_LOGI(TAG, "     RDO Temperature: %.2f", sensor_data.temperature_rdo);
    ESP_LOGI(TAG, "     DO Value: %.2f", sensor_data.do_value);
    ESP_LOGI(TAG, "     PHG Temperature: %.2f", sensor_data.temperature_phg);
    ESP_LOGI(TAG, "     PH Value: %.2f", sensor_data.ph_value);
    // ESP_LOGI(TAG, "     Message: %s", sensor_data.message);

    // memcpy(payload, sensor_data.message, sizeof(sensor_data.message));

    // espnow_data->crc = esp_crc16_le(UINT16_MAX, (uint8_t const *)espnow_data, sizeof(sensor_data_t));

}
table_device_t *recv_data;



static void uart_event(void *pvParameters)
{
    uart_event_t event;
    char data[100];
    size_t buffered_size;
    unsigned char encrypted_message[sizeof(sensor_data_t)];
    unsigned char decrypted_message[sizeof(sensor_data_t)];
   
    while (true){
        if (xQueueReceive(uart0_queue, (void *)&event, (TickType_t)portMAX_DELAY)) {
            // bzero(encrypted_message, 20);
            memset(encrypted_message, 0, event.size);
                ESP_LOGI(TAG, "[Size DATA]: %d", event.size);
                int length = uart_read_bytes(UART_NUM, encrypted_message, event.size, 100 / portTICK_PERIOD_MS);
                uart_flush(UART_NUM);
                ESP_LOGW(TAG, "Reicv %d bytes : ",length);
                printf("%s \n",encrypted_message);
                ESP_LOGW(TAG, "Descrypt: ");
                decrypt_message(encrypted_message,&decrypted_message, sizeof(decrypted_message));
                // decrypted_message[length] = '\0';
                printf("%s \n", decrypted_message);
                // memcpy(decrypted_message, encrypted_message, sizeof(encrypted_message));
                // espnow_data_t *recv_data = (espnow_data_t *)encrypted_message;

                recv_data = (table_device_t *)encrypted_message;
                connect_request* mess=(connect_request*) encrypted_message;
                // print_hex(recv_data,  event.size);

    // In các giá trị cảm biến
    // parse_payload(recv_data);
    // if (!connect_check)
    // accept_connect((uint8_t *)encrypted_message);
    // if (!strncmp(REQUEST_CONNECTION_MSG ,(char *)encrypted_message,sizeof(REQUEST_CONNECTION_MSG))){
    //     dump_uart((uint8_t *)RESPONSE_AGREE,sizeof(RESPONSE_AGREE));
    //     // connect_check=true;
    //     ESP_LOGI(TAG, RESPONSE_AGREE);
    // }
    // if (!strncmp(RESPONSE_CONNECTED ,(char *)encrypted_message,sizeof(RESPONSE_CONNECTED))){
    //     // dump_uart((uint8_t *)RESPONSE_AGREE,sizeof(RESPONSE_AGREE));
    //     connect_check=true;
    //     ESP_LOGI(TAG, "connected");
    // }
    // if ( connect_check)
    // {
    //     ESP_LOGI(TAG, "CONNECTED");
    // }
    // else{
    //     ESP_LOGE(TAG, "DISCONNECTED");
    // }
    
    // if (!connect_check)
    //     if (strcmp((char *)mess, REQUEST_CONNECTION_MSG) == 0) {
    //             connect_check=true;
    //             // time_now=esp_timer_get_time(); 
    //             // time_check=esp_timer_get_time();
    //             ESP_LOGI(TAG, "CONNECTED");
    //             memcpy(mess->message, RESPONSE_AGREE, sizeof(RESPONSE_AGREE));
    //             // dump_uart((const char *)mess->message,  sizeof(mess->message));
    //             uint8_t mac[6];
    //             memcpy(mac, mess->mac, sizeof(mess->mac));
    //             // uint8_t mac[] = mess->mac;
    //             ESP_LOGI("MAC Address", "MAC: %02X:%02X:%02X:%02X:%02X:%02X",
    //                 mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
                
    //     }
                // parse_payload(recv_data);
                //     ESP_LOGI(TAG, "         Message: %s", recv_data->message);

                // parse_payload(&recv_data->payload);

    if (connect_check){
        if (memcmp(&recv_data->peer_addr, &recv_data->peer_addr, 6) == 0) {
            parse_payload(&recv_data->data);
            connect_request keep_connect;
            memcpy(keep_connect.message,MESSAGES_DATA,sizeof(MESSAGES_DATA));
            // dump_uart((const char *)keep_connect.message,  sizeof(keep_connect.message));

            time_now=esp_timer_get_time(); 
            
        }
    }

        }
    }

    vTaskDelete(NULL);
}

void uart_event_task(void){
    // configure_gpio_output();
    // wait_connect_serial();
    xTaskCreate(uart_event, "uart_event", 4096, NULL, 12, NULL);
    // xTaskCreate(check_timeout, "check_timeout", 4096, NULL, 12, NULL);

}


// uint8_t wait_connect_serial(){
//     return connect_check;
// }

void accept_connect(uint8_t *message){
    if (!strncmp(REQUEST_CONNECTION_MSG ,(char *)message,sizeof(REQUEST_CONNECTION_MSG))){
        dump_uart((uint8_t *)RESPONSE_AGREE,sizeof(RESPONSE_AGREE));
        // connect_check=true;
        ESP_LOGI(TAG, RESPONSE_AGREE);
    }
    if (!strncmp(RESPONSE_CONNECTED ,(char *)message,sizeof(RESPONSE_CONNECTED))){
        // dump_uart((uint8_t *)RESPONSE_AGREE,sizeof(RESPONSE_AGREE));
        connect_check=true;
        ESP_LOGI(TAG, "connected");
    }
}

void wait_connect_serial(){
    uint8_t reponse_connect_uart[20]; 
    messages_request mess;
    memcpy(mess.message, REQUEST_UART, sizeof(REQUEST_UART));
    while (true)
    {   
        vTaskDelay(pdMS_TO_TICKS(2000));
        ESP_LOGW(TAG,"wait_connect_serial");
        uart_flush(UART_NUM);
        dump_uart((uint8_t *) &mess.message,  sizeof(mess.message));

        memset(reponse_connect_uart, 0, sizeof(reponse_connect_uart));

        uart_read_bytes(UART_NUM, reponse_connect_uart, sizeof(reponse_connect_uart), pdMS_TO_TICKS(200));
        if (strcmp((char *)reponse_connect_uart, RESPONSE_AGREE) == 0) {
            ESP_LOGI(TAG, "CONNECTED");
            dump_uart((uint8_t *) RESPONSE_CONNECTED,  sizeof(RESPONSE_CONNECTED));
            break;
        }

    }
    return ;
}