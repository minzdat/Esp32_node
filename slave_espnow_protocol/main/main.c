#include <slave_espnow_protocol.h>
<<<<<<< HEAD

void app_main(void)
{
=======
#include "driver/uart.h"
#include "sleep.h"

void app_main(void)
{
    sleep_init(2, false, UART_NUM_1);
>>>>>>> 0d65c9acca272f1193113c0af4e2c3e13a3f601f
    slave_espnow_protocol();
}