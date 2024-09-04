#include "connect_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "string.h"
#include "esp_crt_bundle.h"
#include "esp_http_client.h"
#include "esp_timer.h"
#include <esp_now.h>

// #include "protocol_examples_common.h"

// #include "esp_ping.h"
// #include "esp_ping.h"
// #include "sock/ping_sock.h"


static const char *TAG = "Connect Wifi";
#define WIFI_SSID "Hoai Nam"                                      
#define WIFI_PASS "123456789"
char ip_str[16];
// static EventGroupHandle_t wifi_event_group;
const int CONNECTED_BIT = BIT0;
ip_event_got_ip_t* event;

static INTERNET_CHECK Internet_State;

static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        ESP_LOGW("Connecting","Wifi Connecting");
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        ESP_LOGE("Disconnected","Wifi Disconnected");
        Internet_State=NoInternet;
        esp_wifi_connect();
        // xEventGroupClearBits(wifi_event_group, CONNECTED_BIT);
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        // xEventGroupSetBits(wifi_event_group, CONNECTED_BIT);
        event = (ip_event_got_ip_t*) event_data;
        sprintf(ip_str, IPSTR, IP2STR(&event->ip_info.ip));
        ESP_LOGI(TAG,"Wifi Connected");
        ESP_LOGI(TAG, "Got IP: %s", ip_str);
                // xEventGroupSetBits(wifi_event_group, CONNECTED_BIT);
                // xEventGroupClearBits(wifi_event_group, CONNECTED_BIT);


    }
    else if (event_base == WIFI_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
                ESP_LOGI(TAG,"Wifi Connectedddđd");

    }
}


void wifi_scan();

void wifi_init()
// {
//     ESP_ERROR_CHECK(nvs_flash_init());
//     ESP_ERROR_CHECK(esp_netif_init());
//     ESP_ERROR_CHECK(esp_event_loop_create_default());
//     wifi_event_group = xEventGroupCreate();
    // wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    // ESP_ERROR_CHECK(esp_wifi_init(&cfg));
// }
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);  // Tạo phân vùng NVS, lưu trữ cấu hình wifi, broker mqtt,...
    ESP_ERROR_CHECK(esp_netif_init());  // Thiết lập giao tiếp mạng (Wifi, LAN, PPoPE,...)
    ESP_ERROR_CHECK(esp_event_loop_create_default());  // Thiết lập vòng lặp bắt sự kiện
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

}

wifi_config_t ap_conf;
void wifi_init_sta(const char* w_ssid,const char* w_pass) {

    // wifi_event_group = xEventGroupCreate();

    //     wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    // ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    esp_netif_create_default_wifi_sta();

    //     wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    // ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    // esp_log_level_set("wifi",ESP_LOG_NONE);
    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    //                 ESP_LOGI(TAG,"Wifi Connectedddđd");

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        &instance_got_ip));
    // //                                                                     ESP_LOGI(TAG,"Wifi Connectedddđd");

    // esp_wifi_set_channel(3, WIFI_SECOND_CHAN_NONE);



    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS,
        },
    };    
        strcpy((char*)wifi_config.sta.ssid, w_ssid);
        strcpy((char*)wifi_config.sta.password, w_pass);

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));

    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));


       wifi_config_t ap_config = {
        .ap = {
            .ssid = "ABCDEF12345",
            .ssid_len = strlen("ABCDEF12345"),
            .channel = 3,
            .authmode = WIFI_AUTH_WPA_WPA2_PSK,
            .password = "123456789",
            .max_connection = 4,
            .beacon_interval = 100,
        },
    };
    esp_wifi_set_config(WIFI_IF_AP, &ap_config);
    ESP_ERROR_CHECK(esp_wifi_start());
    //  wifi_scan();

        esp_wifi_set_channel(3, WIFI_SECOND_CHAN_NONE);
        ap_conf=ap_config;
        
    // esp_wifi_connect();
    // xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT, false, true, portMAX_DELAY);
}

void print_ap_channel(){
    ESP_LOGI(TAG, "AP Channel: %d", ap_conf.ap.channel);
}

void print_wifi_channel() {
    wifi_ap_record_t ap_info;
    esp_err_t err = esp_wifi_sta_get_ap_info(&ap_info);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "Current Wi-Fi Channel: %d", ap_info.primary);
    } else {
        ESP_LOGE(TAG, "Failed to get AP info: %s", esp_err_to_name(err));
    }
}


void wifi_scan() {
    uint16_t number = 0;
    wifi_ap_record_t app_info[20];

    // Scan Wi-Fi
    esp_wifi_scan_start(NULL, true);
    esp_wifi_scan_get_ap_records(&number, app_info);
        ESP_LOGE(TAG, "number: %d",number);

    for (int i = 0; i < number; i++) {
        ESP_LOGI(TAG, "SSID: %s, Channel: %d", app_info[i].ssid, app_info[i].primary);
    }
}

// void wifi_init_sta(const char* w_ssid,const char* w_pass) {
//     esp_netif_create_default_wifi_sta();
//     wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
//     ESP_ERROR_CHECK(esp_wifi_init(&cfg));
//     // esp_log_level_set("wifi",ESP_LOG_NONE);
//     esp_event_handler_instance_t instance_any_id;
//     esp_event_handler_instance_t instance_got_ip;
//                     ESP_LOGI(TAG,"Wifi gg");

//     ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
//                                                         ESP_EVENT_ANY_ID,
//                                                         &wifi_event_handler,
//                                                         NULL,
//                                                         &instance_any_id));
//     ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
//                                                         IP_EVENT_STA_GOT_IP,
//                                                         &wifi_event_handler,
//                                                         NULL,
//                                                         &instance_got_ip));
//     ESP_LOGI(TAG,"Wifi wifi_config");


//     wifi_config_t wifi_config = {
//         .sta = {
//             .ssid = WIFI_SSID,
//             .password = WIFI_PASS,
//         },
//     };    
//         strcpy((char*)wifi_config.sta.ssid, w_ssid);
//         strcpy((char*)wifi_config.sta.password, w_pass);

//     ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
//     ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
//     ESP_ERROR_CHECK(esp_wifi_start());

//     ESP_LOGI(TAG,"Wifi esp_wifi_start");
//     // esp_wifi_connect();

//     // xEventGroupWaitBits(g_wifi_event_group, g_constant_ConnectedBit, true, true, portMAX_DELAY);
//     xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT, false, true, portMAX_DELAY);

    // xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT, false, true, portMAX_DELAY);
//         ESP_LOGI(TAG,"Wifi wifi_event_group");

// }



void wifi_init_softap(const char* w_ssid,const char* w_pass) {

    esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);
    esp_wifi_set_mode(WIFI_MODE_AP);
    wifi_config_t wifi_config = {
        .ap = {
            .ssid = "",
            .ssid_len = strlen(w_ssid),
            .channel = 1,
            .password = "",
            .max_connection = 10,
            .authmode = WIFI_AUTH_WPA_WPA2_PSK
        },
    };

  
        strcpy((char*)wifi_config.ap.ssid, w_ssid);
        strcpy((char*)wifi_config.ap.password, w_pass);    
    esp_wifi_set_config(WIFI_IF_AP, &wifi_config);
    esp_wifi_start();
}




static void http_cleanup(esp_http_client_handle_t client)
{
    esp_http_client_close(client);
    esp_http_client_cleanup(client);
}



// https_requestt();
void https_request_check(const char* urrl){
    int log=0;
    static bool log_esp = true;

    esp_http_client_handle_t client=NULL;
    // esp_http_client_config_t config={};
    esp_http_client_config_t config = {
        .url = urrl,
        .crt_bundle_attach = esp_crt_bundle_attach,
        .timeout_ms = 5000,
        .keep_alive_enable = true,
        };
    client = esp_http_client_init(&config);

    int64_t start_time = esp_timer_get_time();
    // Perform the request
    esp_err_t err = esp_http_client_perform(client);
    // Record the end time
    int64_t end_time = esp_timer_get_time();
    // Calculate the elapsed time in milliseconds
    int64_t time_ms = (end_time - start_time) / 1000;
    if (err == ESP_OK) {
        if (log_esp){
        printf("________________________________\n");
        ESP_LOGI(TAG,"Reply from: %s",urrl);
        ESP_LOGI(TAG, "HTTPS Status = %d",esp_http_client_get_status_code(client));
        ESP_LOGI(TAG, " content_length = %"PRId64  ,esp_http_client_get_content_length(client));
        ESP_LOGI(TAG,"ms: %"PRId64 ,time_ms);
        printf("________________________________\n");
        }
        Internet_State=Internet;
        // return 0;
    } 
    else {
        if (log_esp){
        ESP_LOGE(TAG, "HTTP GET request failed: %s", esp_err_to_name(err));
        }
        Internet_State=NoInternet;
    }
    http_cleanup(client);
    vTaskDelay(pdMS_TO_TICKS(5000));
}
void https_pingg(void *pvParameters){
    // configure_client(&client);
    char *urrl = (char *)pvParameters;
    while (1)
    {
    https_request_check(urrl);
    }
}

void https_ping(const char* urrl){
    xTaskCreate(&https_pingg, "https_pingg", 4096, urrl, 5, NULL);
}

int check_internet(void)
{
    return Internet_State;
}


void set_wifi_max_tx_power(int rssid) {
    esp_err_t err = esp_wifi_set_max_tx_power(rssid*4);
    // Kiểm tra và log kết quả
    ESP_LOGE(TAG, "Esp set power: %s", esp_err_to_name(err));

    int8_t tx_power;
    esp_wifi_get_max_tx_power(&tx_power);
    tx_power= tx_power*0.25;
    ESP_LOGI(TAG, "Current max TX power: %d dBm", tx_power);
}