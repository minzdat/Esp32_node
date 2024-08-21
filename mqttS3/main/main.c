#include <esp_now.h>
#include <esp_wifi.h>
#include <esp_log.h>
#include <esp_system.h>
#include <nvs_flash.h>
#include <string.h>
#include <esp_mac.h>
#include "esp_timer.h"
#include <math.h>


#include "connect_wifi.h"
#include "pub_sub_client.h"
#include "read_serial.h"
static const char *TAG = "ESP-NOW Master";


int start=0;
int stop=0;
int time_s=0;

typedef struct {
        char data[250];
} esp_now_message_t;

uint8_t lmk[16] = {0x04, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10};
uint8_t pmk[16] = {0x03, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10};
uint8_t broadcast_mac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

esp_now_peer_info_t peer_info = {};
esp_now_peer_info_t peer_broadcast = {};

int8_t rssi;
// uint8_t mac_a[6] ={0xDC, 0xDA, 0x0C, 0x0D, 0x41, 0xAC};
// uint8_t mac_a[6] ={0xDC, 0xDA, 0x0C, 0x0D, 0x41, 0x64};

    float angle=-100;
    float sin_angle;
    float sin_angle2;
    float sin_angle3;
    float sin_angle4;

void send_data(){
    char data[100];
    ESP_LOGI(TAG,"Receive data from queue successfully");
            sin_angle = sin(angle*0.06283)*10+10;
            sin_angle2 = sin(angle*0.031415)*10+10;
            sin_angle3 = sin(angle*0.0157)*10+10;
            sin_angle4 = sin(angle*0.031415)*10+30;

            angle++;
            if (angle>100){
                angle=-100;
            }
            sprintf(data, "temperature_rdo: %f, do: %f, temperature_phg: %f, ph: %f",sin_angle,sin_angle2,sin_angle3,sin_angle4);
            // xEventGroupWaitBits(g_wifi_event, g_constant_wifi_connected_bit, pdFALSE, pdTRUE, portMAX_DELAY);
            //g_index_queue=0;
            data_to_mqtt(data, "v1/devices/me/telemetry",500, 1);
}
static void mqtt_task(void *pvParameters)
{
    // char data[100];
    // SensorsData sensor;
    // BaseType_t ret;
    // float angle=-100;
    // float sin_angle;
    // float sin_angle2;
    // float sin_angle3;
    // float sin_angle4;
    while(1)
    {
        // ret=xQueueReceive(g_publisher_queue,&sensor,(TickType_t)portMAX_DELAY);
        if(true)
        {
            send_data();
            
                    vTaskDelay(5000/ portTICK_PERIOD_MS);

        }
    }
    vTaskDelete(NULL);
}

#define SSID "Aruba_Wifi"
#define PASS "123456789"
#define BROKER "mqtt://35.240.204.122:1883"
#define USER_NAME "tts-3"
#define TOPIC "v1/devices/me/rpc/request/+"
#define MAX_RSSI 20
void app_main(void) {

    wifi_init();
    wifi_init_sta(SSID,PASS);

    uart_config();
    uart_event_task();

    mqtt_init(BROKER, USER_NAME, NULL);
    subcribe_to_topic(TOPIC,2);
    //  xTaskCreate(mqtt_task, "mqtt_task", 5000, NULL, 5, NULL);

}
