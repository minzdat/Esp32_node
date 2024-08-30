#include <master_espnow_protocol.h>



// uint8_t request_connect_uart[]="request_connect";
// uint8_t reponse_connect_uart[20];
// #define RESPONSE_AGREE_CONNECT      "AGREE_connect"


void app_main(void)
{
    // memcpy(mess.message, request_connect_uart, sizeof(request_connect_uart));
    // esp_read_mac(mess.mac, 1);

    uart_config();

    // wait_connect_serial();
    // check_timeout();
    master_espnow_protocol();

    uart_event_task();
}