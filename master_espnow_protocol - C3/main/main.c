#include <master_espnow_protocol.h>
#include "uart.h"


void app_main(void)
{
    uart_config();
    // uart_event_task();
    master_espnow_protocol();
    //     while (1)
    // {
    //     add_json();
    //     vTaskDelay(pdMS_TO_TICKS(1000));
    // }
    
}