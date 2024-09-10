#include <master_espnow_protocol.h>
#include <master_controller.h>
#include <read_serial.h>

void app_main(void)
{
    uart_config();

    uart_event_task();

    master_espnow_protocol();
}