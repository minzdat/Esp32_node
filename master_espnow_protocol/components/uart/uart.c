#include "uart.h"
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "freertos/queue.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "uart.h"
#include "cJSON.h"
#include "esp_crc.h"

// #define PATTERN_CHR_NUM    (3)         /*!< Set the number of consecutive and identical characters received by receiver which defines a UART pattern*/
#define UART1_NUM         UART_NUM_1    // Sử dụng UART1
#define UART_BAUD_RATE     (115200)
#define TASK_STACK_SIZE    (1024*2)
#define QUEUE_SIZE         (20)
#define TXD1_PIN           (5)    // Chân TX (thay đổi nếu cần)
#define RXD1_PIN           (4)    // Chân RX (thay đổi nếu cần)

#define PATTERN_CHR_NUM    (3)  
static const char *TAG = "UART TEST";

int count = 0;
int count1 = 1;
int count_num =0;

#define BUF_SIZE (1024*4)
#define RD_BUF_SIZE        (BUF_SIZE)
static QueueHandle_t uart1_queue;

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
            ESP_LOGI(TAG, "uart[%d] event", UART1_NUM);
            switch (event.type) {
            case UART_DATA:
                ESP_LOGI(TAG, "[UART DATA]: %d\n", event.size);
                uart_read_bytes(UART1_NUM, dtmp, event.size, portMAX_DELAY);
                dtmp[event.size] = '\0';  // Đảm bảo chuỗi kết thúc
                //decrypt_message(dtmp, decrypted_dtmp, event.size);
                ESP_LOGI(TAG, "[DATA EVT]: %s", dtmp);
                //parse_json((const char*)dtmp);
                //uart_read_bytes(UART1_NUM, dtmp, BUF_SIZE, portMAX_DELAY);
                //dtmp[BUF_SIZE] = '\0';  // Đảm bảo chuỗi kết thúc
                if (strcmp((char*)dtmp, "restart") == 0) {
                    count_num++;
                    count1--;
                    ESP_LOGE(TAG, "Button, count: %d", count_num);
                    esp_restart();
                }
                count1++;
                //ESP_LOGI(TAG, "[DATA EVT]:");
                //uart_write_bytes(UART1_NUM, (const char*) dtmp, event.size);
                //uart_write_bytes(UART1_NUM, (const char*) dtmp, BUF_SIZE);
                
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
void uart_config(void){
    esp_log_level_set(TAG, ESP_LOG_INFO);
        uart_config_t uart_config1 = {
        .baud_rate = UART_BAUD_RATE ,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    int intr_alloc_flags = 0;
    ESP_ERROR_CHECK(uart_driver_install(UART1_NUM, BUF_SIZE * 4, BUF_SIZE * 4, QUEUE_SIZE, &uart1_queue, 0)); //uart_driver_install(uart_port_t uart_num, int rx_buffer_size, int tx_buffer_size, int queue_size, QueueHandle_t *uart_queue, int intr_alloc_flags)
    ESP_ERROR_CHECK(uart_param_config(UART1_NUM, &uart_config1));
    esp_log_level_set(TAG, ESP_LOG_INFO);
    ESP_ERROR_CHECK(uart_set_pin(UART1_NUM, TXD1_PIN, RXD1_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE)); //uart_set_pin(uart_port_t uart_num, int tx_io_num, int rx_io_num, int rts_io_num, int cts_io_num)
    uart_enable_pattern_det_baud_intr(UART1_NUM, '+', PATTERN_CHR_NUM, 9, 0, 0);
    uart_pattern_queue_reset(UART1_NUM, QUEUE_SIZE);
    
    xTaskCreate(uart_event_task, "uart_event_task", 1024*4, NULL, 12, NULL);
}

int sendStructData(const char* logName, const void* data, size_t size)
{
    if (data == NULL) {
        ESP_LOGE(logName, "Data is NULL");
        return -1;
    }
    const int txBytes = uart_write_bytes(UART1_NUM, data, size);
    if (txBytes < 0) {
        ESP_LOGE(logName, "Failed to write bytes: %d", txBytes);
    } else {
        ESP_LOGE(logName, "Wrote %d bytes", txBytes);
    }
    return txBytes;
}
uint16_t calculate_crc16(const uint8_t *data, size_t len) {
    return esp_crc16_le(UINT16_MAX, data, len);
}

void send_uart(sensor_data_tt *data, const char *message) {
    // Khởi tạo cấu trúc dữ liệu để gửi
    sensor_data_uart_t packet;
    sensor_data_uart_t *packet_ptr = &packet; // Con trỏ đến cấu trúc gửi

    // Sao chép dữ liệu vào gói tin
    memcpy(&packet_ptr->data, data, sizeof(sensor_data_tt));
    strncpy(packet_ptr->message, message, sizeof(packet_ptr->message) - 1);
    packet_ptr->message[sizeof(packet_ptr->message) - 1] = '\0'; // Đảm bảo kết thúc chuỗi

    // Tính toán CRC cho gói tin
    size_t len = sizeof(packet_ptr->data) + sizeof(packet_ptr->message);
    uint16_t crc = esp_crc16_le(UINT16_MAX, (const uint8_t *)packet_ptr, len);
    packet_ptr->crc = crc;

    // Tạo bộ đệm để gửi dữ liệu và CRC
    size_t packet_size = sizeof(packet_ptr->data) + sizeof(packet_ptr->message) + sizeof(packet_ptr->crc);
    uint8_t buffer[packet_size];
    memcpy(buffer, packet_ptr, packet_size);
    count ++;
    // Gửi dữ liệu qua UART
    int txBytes = uart_write_bytes(UART1_NUM, (const char *)buffer, packet_size);

    // In chiều dài gói tin gửi
    printf("Sent %d bytes (data + CRC), s: %d, r: %d\n", txBytes, count, count1);
}

