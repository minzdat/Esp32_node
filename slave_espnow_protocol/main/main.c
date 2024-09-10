#include <slave_espnow_protocol.h>
#include <light_sleep.h>
#include <slave_controller.h>

void app_main(void)
{
    light_sleep_init();

    slave_controller_init();

    slave_espnow_protocol();

}