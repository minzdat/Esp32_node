#include <slave_espnow_protocol.h>
#include "driver/uart.h"
#include "sleep.h"

void app_main(void)
{
    sleep_init(2, false, UART_NUM_1);
    slave_espnow_protocol();
}