
#include "uart.h"
static const char *TAG = "UART TEST";
static void uart_event_task(void *pvParameters)
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
            ESP_LOGI(TAG, "uart[%d] event:", UART1_NUM);
            switch (event.type) {
            case UART_DATA:
                ESP_LOGI(TAG, "[UART DATA]: %d\n", event.size);
                int length = uart_read_bytes(UART1_NUM, recv_message, event.size, portMAX_DELAY);
                process_uart_data(recv_message, length);
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
void app_main(void)
{
    uart_init();
    //xTaskCreate(parsedata, "parsedata", 1024*4, NULL, configMAX_PRIORITIES - 2, NULL);
    xTaskCreate(uart_event_task, "uart_event_task", 1024*4, NULL, 12, NULL);
    //xTaskCreate(rx1_task, "uart1_rx_task", 1024*4, NULL, configMAX_PRIORITIES - 1, NULL);
    //xTaskCreate(tx1_task, "uart1_tx_task", 1024*2, NULL, configMAX_PRIORITIES - 2, NULL);
}