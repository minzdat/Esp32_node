#include <slave_espnow_protocol.h>
#include "read_serial.h"

void app_main(void)
{
    uart_config();
    uart_event_task();
    slave_espnow_protocol();

    //     while (1)
    // {
    //     str_json_data();
    //     vTaskDelay(pdMS_TO_TICKS(1000));
    // }
}