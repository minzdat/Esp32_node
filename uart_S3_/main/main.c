#include "uart.h"
#include "button_.h"
#include "Wifi_.h"
#include "esp_wifi.h"
#include "nvs_flash.h"
#include "pub_sub_client.h"
#include "lwip/ip4_addr.h"  // Để sử dụng ip4_addr_t
static const char *TAG = "UART TEST";

int count = 0;


#define SSID "Aruba_Wifi"
#define PASS "123456789"
#define BROKER "mqtt://35.240.204.122:1883"
#define USER_NAME_1 "tts-1"
#define USER_NAME_3 "tts-3"
#define TOPIC "v1/devices/me/rpc/request/+"
#define MAX_RSSI 20
#define MAXIMUM_RETRY   5
static int s_retry_num = 0;

static EventGroupHandle_t s_wifi_event_group;

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1



void app_main(void)
{
    wifi_init();
    wifi_init_sta(SSID, PASS);
    // wifi_init_sta();
    uart_init();
    button_init();
    mqtt_init(BROKER, USER_NAME_1, NULL);
    mqtt_init2(BROKER, USER_NAME_3, NULL);
    //mqtt_init(BROKER, USER_NAME, NULL);
    subcribe_to_topic(TOPIC,1);

    //xTaskCreate(parsedata, "parsedata", 1024*4, NULL, configMAX_PRIORITIES - 2, NULL);
    xTaskCreate(uart_event_task, "uart_event_task", 1024*4, NULL, 12, NULL);
    //xTaskCreate(rx1_task, "uart1_rx_task", 1024*4, NULL, configMAX_PRIORITIES - 1, NULL);
    //xTaskCreate(tx1_task, "uart1_tx_task", 1024*2, NULL, configMAX_PRIORITIES - 2, NULL);
    
    //xTaskCreate(datatomqtt, "mqtt_task", 5000, NULL, 5, NULL);
    
    //esp_mqtt_client_disconnect(USER_NAME_1);
    // mqtt_init(BROKER, USER_NAME_3, NULL);
    // xTaskCreate(datatomqtt, "mqtt_task", 5000, NULL, 5, NULL);
    // esp_mqtt_client_disconnect(USER_NAME_3);
}