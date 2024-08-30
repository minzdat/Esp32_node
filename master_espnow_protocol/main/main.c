#include <master_espnow_protocol.h>

void app_main(void)
{
    // uart_config();
    uart_config();

    master_espnow_protocol();
    // uart_event_task();
    uart_event_task();
}