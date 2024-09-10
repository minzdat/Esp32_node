#include <master_espnow_protocol.h>
table_device_t table_devices[MAX_SLAVES];

void app_main(void)
{
    uart_config();
    uart_event_task();
    master_espnow_protocol();
}