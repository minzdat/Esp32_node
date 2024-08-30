#include "read_serial.h"

const char *TAG="Read Serial";
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "freertos/queue.h"
#include "driver/uart.h"
#include "driver/gpio.h"

#include "cJSON.h"

// #define PATTERN_CHR_NUM    (3)         /*!< Set the number of consecutive and identical characters received by receiver which defines a UART pattern*/
#define UART_NUM_P2         UART_NUM_1    // Sử dụng UART1
#define TX_GPIO_NUM     5    // Chân TX (thay đổi nếu cần)
#define RX_GPIO_NUM      4    // Chân RX (thay đổi nếu cần)
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
    uart_driver_install(UART_NUM_P2, BUF_SIZE, BUF_SIZE, 0, NULL, 0);

    // uart_driver_install(UART_NUM_P2, BUF_SIZE * 2, BUF_SIZE * 2, 20, &uart0_queue, 0);
    uart_param_config(UART_NUM_P2, &uart_config);
    // uart_set_pin(UART_NUM_P2, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    uart_set_pin(UART_NUM_P2, TX_GPIO_NUM, RX_GPIO_NUM, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    // uart0_queue = xQueueCreate(10, BUF_SIZE);

    // uart_set_pin(EX_UART_NUM_P2, TX_PIN, RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
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

void dump_uart(sensor_data_tt *message){
    
    size_t len = sizeof(sensor_data_tt);
    printf("send \n");
    uint8_t encrypted_message[len]; // AES block size = 16 bytes
    // Mã hóa tin nhắn
    encrypt_message((unsigned char *)message, encrypted_message, len);
    // uart_write_bytes(UART_NUM_P2, (const char *)message, sizeof(sensor_data_t));
    uart_write_bytes(UART_NUM_P2, (unsigned char *)encrypted_message, len);
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
    uart_write_bytes(UART_NUM_P2,json_string, strlen(json_string));


}


static void uart_event(void *pvParameters)
{
    uart_event_t event;
    size_t buffered_size;
    uint8_t* dtmp = (uint8_t*) malloc(RD_BUF_SIZE);
   
    while (true){
        if (xQueueReceive(uart0_queue, (void *)&event, (TickType_t)portMAX_DELAY)) {
            // bzero(dtmp, RD_BUF_SIZE);
            memset(dtmp, 0, RD_BUF_SIZE);

                    // ESP_LOGI(TAG, "[Size DATA]: %d", event.size);
                    uart_read_bytes(UART_NUM_P2, dtmp, event.size, portMAX_DELAY);
                    ESP_LOGW(TAG, "Reicv %d bytes : ",event.size);
                    printf("%s \n",dtmp);
                    // send(sock, dtmp,event.size, 0);
                    // uart_write_bytes(EX_UART_NUM_P2, (const char*) dtmp, event.size);
                    // printe("bufferacbd");

        }
    }
    free(dtmp);
    dtmp = NULL;
    vTaskDelete(NULL);
}

void uart_event_task(void){
    xTaskCreate(uart_event, "uart_event", 4096, NULL, 12, NULL);
}