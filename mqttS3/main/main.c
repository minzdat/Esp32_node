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
#include "iot_button.h"

static const char *TAG = "ESP-NOW Master";


// int start=0;
// int stop=0;
// int time_s=0;

typedef struct {
        char data[250];
} esp_now_message_t;
table_device_t table_devices[MAX_SLAVES];

uint8_t lmk[16] = {0x04, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10};
uint8_t pmk[16] = {0x03, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10};
uint8_t broadcast_mac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

esp_now_peer_info_t peer_info = {};
esp_now_peer_info_t peer_broadcast = {};

int8_t rssi;
uint16_t data2mqtt[120];
// uint8_t mac_a[6] ={0xDC, 0xDA, 0x0C, 0x0D, 0x41, 0xAC};
// uint8_t mac_a[6] ={0xDC, 0xDA, 0x0C, 0x0D, 0x41, 0x64};

    float angle=-100;
    float temperature_rdo;
    float ph_value;
    float temperature_phg;
    float do_value;


int compare_mac_addresses(const char *mac_s, const uint8_t *mac_m) {
    uint8_t mac_bytes[6];
    int values[6];
    
    // Parse the string MAC address
    if (sscanf(mac_s, "%x:%x:%x:%x:%x:%x", &values[0], &values[1], &values[2],
               &values[3], &values[4], &values[5]) != 6) {
        return 0; // Invalid format
    }
    
    // Convert parsed values to bytes
    for (int i = 0; i < 6; i++) {
        mac_bytes[i] = (uint8_t)values[i];
    }
    
    // Compare the two MAC addresses
    return memcmp(mac_bytes, mac_m, 6) == 0;
}

void send_data(table_device_t sensor_data){
    char data[200];
    // uint8_t data_u[100];
    ESP_LOGI(TAG,"Receive data from queue successfully");
            // get_data(&temperature_rdo, &do_value, &temperature_phg, &ph_value);     //temperature_rdo  do_value temperature_phg ph_value
            sprintf(data, "temperature_rdo: %f, do: %f, temperature_phg: %f, ph: %f, cpu_temp: %f ",sensor_data.data.temperature_rdo,sensor_data.data.do_value,sensor_data.data.temperature_phg, sensor_data.data.ph_value, sensor_data.data.temperature_mcu);
            // xEventGroupWaitBits(g_wifi_event, g_constant_wifi_connected_bit, pdFALSE, pdTRUE, portMAX_DELAY);
            //g_index_queue=0;
            data_to_mqtt(data, "v1/devices/me/telemetry",500, 1);
}

void mqtt_subcriber(esp_mqtt_event_handle_t event)
{
    char data_receiv[50];
    char data[200];
    strncpy(data_receiv,event->data,event->data_len);
    data_receiv[event->data_len] = '\0';
            ESP_LOGI(TAG, "Other event id:%d", event->event_id);
            ESP_LOGI(TAG, "Other event len:%d", event->data_len);
            ESP_LOGI(TAG, "Other event data:%s",data_receiv);
    cJSON* data_sub=cJSON_Parse(data_receiv);
    cJSON *params=cJSON_GetObjectItem(data_sub,"params");
    cJSON *command = cJSON_GetObjectItem(params, "abc");
    cJSON *messages = cJSON_GetObjectItem(params, "messages");
    cJSON *mac_j = cJSON_GetObjectItem(params, "mac");

    if (command != NULL) 
        ESP_LOGI(TAG,"ABC: %d\n", command->valueint);

    ESP_LOGI(TAG,"Topic: %s\n",event->topic);
    if ((messages != NULL)&&(mac_j != NULL)){
        ESP_LOGI(TAG,"Messages: %s\n", messages->valuestring);
        ESP_LOGI(TAG,"MAC: %s\n", mac_j->valuestring);
        const char *mac_s = mac_j->valuestring;
    for (int i = 0; i < MAX_SLAVES; i++){
        // if (memcmp(mess_get->mac, table_devices[i].peer_addr, 6)==0){
        if (compare_mac_addresses(mac_s, table_devices[i].peer_addr)) {
        sprintf(data, "temperature_rdo: %f, do: %f, temperature_phg: %f, ph: %f, cpu_temp: %f ",table_devices[i].data.temperature_rdo,table_devices[i].data.do_value,table_devices[i].data.temperature_phg, table_devices[i].data.ph_value, table_devices[i].data.temperature_mcu);
    }
    }
    response_mqtt(data,event->topic);
    }

}
QueueHandle_t g_mqtt_queue;
connect_request mess_button;

static void mqtt_task(void *pvParameters)
{
    sensor_data_t sensor_data;
    connect_request mess_getdata;
    table_device_t res_getdata;

    uint8_t mac_m[] = {0xf4, 0x12, 0xfa, 0x42, 0xa3, 0xdc};
    memcpy(mess_getdata.message, GET_DATA, sizeof(GET_DATA));
    memcpy(mess_getdata.mac, mac_m, 6);
    while(1){
        vTaskDelay(5000/ portTICK_PERIOD_MS);

        if (!wait_wake_up()) {
            ESP_LOGE(TAG, "Failed to wake up");
            continue;
        }
        dump_uart(&mess_getdata, sizeof(mess_getdata));
        int ret =get_uart(&res_getdata,sizeof(res_getdata),500);
        if (ret){
            parse_payload(&res_getdata.data);
            // xQueueReceive(g_mqtt_queue,&sensor_data,(TickType_t)portMAX_DELAY);
            // if(xQueueReceive(g_mqtt_queue,&sensor_data,500/ portTICK_PERIOD_MS)){
                send_data(res_getdata);
            // }
        }
        else {
            ESP_LOGE(TAG, "Failed to get_uart");
            continue;

        }

    }
    vTaskDelete(NULL);
}




static void button_longpress_cb(void *arg, void *usr_data)
{
    // memcpy(mess_button.message, BUTTON_MSG, sizeof(BUTTON_MSG));
    memcpy(mess_button.message, BUTTON_MSG, sizeof(BUTTON_MSG));
    // dump_uart(&mess_button, sizeof(mess_button));
    wait_wake_up();
    // delay(500);
        get_table();
    log_table_devices();
    
    ESP_ERROR_CHECK(!(BUTTON_LONG_PRESS_START == iot_button_get_event(arg)));
    dump_uart(&mess_button, sizeof(mess_button));
    ESP_LOGI(TAG, "long press");
}

#define LONG_PRESS_TIME_MS 1000
#define SHORT_PRESS_TIME_MS 100
void button_init(){
    // create gpio button
    button_config_t button_config = {
        .type = BUTTON_TYPE_GPIO,
        .long_press_time = LONG_PRESS_TIME_MS,
        .short_press_time = SHORT_PRESS_TIME_MS,
        .gpio_button_config = {
            .gpio_num = 11,
            .active_level = 0,
        },
    };
    button_handle_t gpio_btn = iot_button_create(&button_config);
    if(NULL == gpio_btn) {
        ESP_LOGE(TAG, "Button create failed");
    }
    button_handle_t button_handle = iot_button_create(&button_config);
    // iot_button_register_cb(button_handle, BUTTON_SINGLE_CLICK, button_single_cb, NULL);
    // iot_button_register_cb(button_handle, BUTTON_DOUBLE_CLICK, button_double_cb, NULL);
    iot_button_register_cb(button_handle, BUTTON_LONG_PRESS_START, button_longpress_cb, NULL);
}

#define SSID "Aruba_Wifi"
#define PASS "123456789"
#define BROKER "mqtt://35.240.204.122:1883"
#define USER_NAME "tts-2"
// #define USER_NAME "envisor_testing_13"
#define TOPIC "v1/devices/me/rpc/request/+"
#define MAX_RSSI 20
void app_main(void) {
    g_mqtt_queue = xQueueCreate(10, sizeof(sensor_data_t));

    button_init();
    uart_config();
    uart_event_task();
    // while (true){   
    //     if (wait_connect_serial())
    //     break;
    //     delay(100);
    // }

    wifi_init();
    wifi_init_sta(SSID,PASS);

    

    mqtt_init(BROKER, USER_NAME, NULL);
    subcribe_to_topic(TOPIC,1);
    xTaskCreate(mqtt_task, "mqtt_task", 5000, NULL, 5, NULL);

    // data_read=0;

}
