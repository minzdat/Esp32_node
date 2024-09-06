#include "master_espnow_protocol.h"
#include "uart.h"
void test(){
    while(true){
        send_uart_(UART1_NUM, "CONNECT");
        vTaskDelay(5000 / portTICK_PERIOD_MS);
    }
}
void app_main(void)
{
    uart_config();
    master_espnow_protocol();
    //send_uart_(UART1_NUM, "CONNECT");
    //send_uart_message("helo", sizeof("helo"));
    //xTaskCreate(test, "hhh",5000, NULL, 5, NULL );
}