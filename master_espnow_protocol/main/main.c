<<<<<<< HEAD
#include <master_espnow_protocol.h>

void app_main(void)
{
    uart_config();
    uart_event_task();
    master_espnow_protocol();
=======
#include "master_espnow_protocol.h"
#include "uart.h"
#include "sleep.h"
void test(){
    while(true){
        send_uart_(UART1_NUM, "CONNECT");
        vTaskDelay(5000 / portTICK_PERIOD_MS);
    }
}
void app_main(void)
{
    //register_timer_wakeup(3);
    uart_config();
    master_espnow_protocol();
    
    //esp_sleep_enable_timer_wakeup(3*1000*1000);  // Thời gian sleep tính bằng microseconds
    //esp_light_sleep_start();
    //send_uart_(UART1_NUM, "CONNECT");
    //send_uart_message("helo", sizeof("helo"));
    //xTaskCreate(test, "hhh",5000, NULL, 5, NULL );
    //ESP_LOGE("44", "System woke up and continues execution.");
>>>>>>> 0d65c9acca272f1193113c0af4e2c3e13a3f601f
}