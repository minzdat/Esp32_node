#include "master_espnow_protocol.h"
#include "uart.h"

void app_main(void)
{
    uart_config();
    master_espnow_protocol();
    
    //send_uart_message("helo", sizeof("helo"));
}