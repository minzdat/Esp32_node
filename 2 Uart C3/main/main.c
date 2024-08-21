/* UART Echo Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "sdkconfig.h"
#include "esp_log.h"
#include "cJSON.h"
#include "mbedtls/aes.h"
/*
 * - Port: configured UART
 * - Receive (Rx) buffer: on
 * - Transmit (Tx) buffer: off
 * - Flow control: off
 * - Event queue: off
 * - Pin assignment: see defines below (See Kconfig)
 */

#define UART0_NUM          UART_NUM_0 //defauft - flash
#define UART1_NUM          UART_NUM_1
#define UART_BAUD_RATE     (115200)
#define TASK_STACK_SIZE    (1024*2)
#define QUEUE_SIZE         (20)

#define TXD1_PIN           (5)
#define RXD1_PIN           (4)
#define RTS1_PIN           (9)
#define CTS1_PIN           (8)

static const char *TAG = "UART TEST";
static const char *EXAMPLE_TAG = "JSON";

#define PATTERN_CHR_NUM    (3)         /*!< Set the number of consecutive and identical characters received by receiver which defines a UART pattern*/

#define BUF_SIZE (1024*4)
#define RD_BUF_SIZE        (BUF_SIZE)
static QueueHandle_t uart1_queue;

void uart_init(void);
void uart_read_task(void *param);
void parse_json(const char *json_str);
/*Struct data----------------------------------*/
typedef struct {
    uint8_t mac[6];
    float temperature_mcu;
    int rssi;
    float temperature_rdo;
    float do_value;
    float temperature_phg;
    float ph_value;
} sensor_data_t;
// static void echo_task(void *arg)
// {
//     /* Configure parameters of an UART driver,
//      * communication pins and install the driver */
//     uart_config_t uart_config1 = {
//         .baud_rate = UART_BAUD_RATE,
//         .data_bits = UART_DATA_8_BITS,
//         .parity    = UART_PARITY_DISABLE,
//         .stop_bits = UART_STOP_BITS_1,
//         .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
//         .source_clk = UART_SCLK_DEFAULT,
//     };
//     int intr_alloc_flags = 0;

// #if CONFIG_UART_ISR_IN_IRAM
//     intr_alloc_flags = ESP_INTR_FLAG_IRAM;
// #endif

//     ESP_ERROR_CHECK(uart_driver_install(UART1_NUM, BUF_SIZE * 4, 0, 0, NULL, intr_alloc_flags));
//     ESP_ERROR_CHECK(uart_param_config(UART1_NUM, &uart_config1));
//     ESP_ERROR_CHECK(uart_set_pin(UART1_NUM, TXD1_PIN, RXD1_PIN, RTS1_PIN, CTS1_PIN));

//     // Configure a temporary buffer for the incoming data
//     uint8_t *data = (uint8_t *) malloc(BUF_SIZE);

//     while (1) {
//         // Read data from the UART
//         int len = uart_read_bytes(UART1_NUM, data, (BUF_SIZE - 1), 20 / portTICK_PERIOD_MS);
//         // Write data back to the UART
//         uart_write_bytes(UART1_NUM, (const char *) data, len);
//         //uart_write_bytes(UART1_NUM, "hello", sizeof("hello"));
//         if (len) {
//             data[len] = '\0';
//             ESP_LOGI(TAG, "Recv str: %s", (char *) data);
//         }
//     }
// }
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
static void uart_event_task(void *pvParameters)
{
    uart_event_t event;
    size_t buffered_size;
    uint8_t* dtmp = (uint8_t*) malloc(RD_BUF_SIZE + 1);
    if (dtmp == NULL) {
        ESP_LOGE(TAG, "Failed to allocate memory for dtmp");
        vTaskDelete(NULL);
    }
    uint8_t* decrypted_dtmp = (uint8_t*) malloc(RD_BUF_SIZE);  // Bộ đệm để lưu dữ liệu giải mã
    if (decrypted_dtmp == NULL) {
        ESP_LOGE(TAG, "Failed to allocate memory for decrypted_dtmp");
        free(dtmp);
        vTaskDelete(NULL);
    }
    while (true) {
        //Waiting for UART event.
        if (xQueueReceive(uart1_queue, (void *)&event, (TickType_t)portMAX_DELAY)) {
            memset(dtmp, 0, RD_BUF_SIZE);
            ESP_LOGI(TAG, "uart[%d] event:", UART1_NUM);
            switch (event.type) {
            case UART_DATA:
                ESP_LOGI(TAG, "[UART DATA]: %d\n", event.size);
                uart_read_bytes(UART1_NUM, dtmp, event.size, portMAX_DELAY);
                dtmp[event.size] = '\0';  // Đảm bảo chuỗi kết thúc
                decrypt_message(dtmp, decrypted_dtmp, event.size);
                parse_json((const char*)decrypted_dtmp);
                //uart_read_bytes(UART1_NUM, dtmp, BUF_SIZE, portMAX_DELAY);
                //dtmp[BUF_SIZE] = '\0';  // Đảm bảo chuỗi kết thúc
                ESP_LOGI(TAG, "[DATA EVT]: %s", dtmp);
                //ESP_LOGI(TAG, "[DATA EVT]:");
                uart_write_bytes(UART1_NUM, (const char*) dtmp, event.size);
                //uart_write_bytes(UART1_NUM, (const char*) dtmp, BUF_SIZE);
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
                ESP_LOGI(TAG, "uart rx break");
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
void uart_init(void){
    esp_log_level_set(TAG, ESP_LOG_INFO);
    /*Config Uart 1 - Uart 0 is default*/
    uart_config_t uart_config1 = {
        .baud_rate = 115200,
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
    xTaskCreate(uart_event_task, "uart_event_task", 1024*4, NULL, 12, NULL);
}
void parse_json(const char *json_str) {
    cJSON *json = cJSON_Parse(json_str);
    if (json == NULL) {
        ESP_LOGE(EXAMPLE_TAG, "Failed to parse JSON");
        return;
    }

    // Extract values from JSON
    cJSON *temperature_mcu = cJSON_GetObjectItem(json, "temperature_mcu");
    cJSON *temperature_rdo = cJSON_GetObjectItem(json, "temperature_rdo");
    cJSON *dissolved_oxygen = cJSON_GetObjectItem(json, "dissolved_oxygen");
    cJSON *temperature_phg = cJSON_GetObjectItem(json, "temperature_phg");
    cJSON *ph_value = cJSON_GetObjectItem(json, "ph_value");
    cJSON *rssi = cJSON_GetObjectItem(json, "rssi");

    // Print values
    if (cJSON_IsNumber(temperature_mcu)) {
        ESP_LOGI(EXAMPLE_TAG, "Temperature MCU: %.2f", temperature_mcu->valuedouble);
    }
    if (cJSON_IsNumber(temperature_rdo)) {
        ESP_LOGI(EXAMPLE_TAG, "Temperature RDO: %.2f", temperature_rdo->valuedouble);
    }
    if (cJSON_IsNumber(dissolved_oxygen)) {
        ESP_LOGI(EXAMPLE_TAG, "Dissolved Oxygen: %.2f", dissolved_oxygen->valuedouble);
    }
    if (cJSON_IsNumber(temperature_phg)) {
        ESP_LOGI(EXAMPLE_TAG, "Temperature PHG: %.2f", temperature_phg->valuedouble);
    }
    if (cJSON_IsNumber(ph_value)) {
        ESP_LOGI(EXAMPLE_TAG, "pH Value: %.2f", ph_value->valuedouble);
    }
    if (cJSON_IsNumber(rssi)) {
        ESP_LOGI(EXAMPLE_TAG, "RSSI: %d", rssi->valueint);
    }

    // Free memory
    cJSON_Delete(json);
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
            parse_json((char *)data);
        }
    }
    free(data);
}
void app_main(void)
{
    uart_init();
    //xTaskCreate(rx1_task, "uart1_rx_task", 1024*4, NULL, configMAX_PRIORITIES - 1, NULL);
    //xTaskCreate(tx1_task, "uart1_tx_task", 1024*2, NULL, configMAX_PRIORITIES - 2, NULL);
}