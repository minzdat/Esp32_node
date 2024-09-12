#include "master_espnow_protocol.h"
#include "light_sleep.h"
#include "master_controller.h"
// #include "read_serial.h"

void app_main(void)
{
    light_sleep_init();

    // uart_config();
    // uart_event_task();

    master_espnow_protocol();
}