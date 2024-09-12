#include "global.h"
temperature_sensor_handle_t g_cpu_temp_sensor = NULL;
void get_device_init_parameter(device_parameter_t *device_parameter)
{
    wifi_config_t wifi_cfg;
    esp_wifi_get_config(ESP_IF_WIFI_STA, &wifi_cfg);
    strncpy(device_parameter->internet, (char *)wifi_cfg.sta.ssid,strlen((char *)wifi_cfg.sta.ssid));
    device_parameter->internet[strlen((char *)wifi_cfg.sta.ssid)] = '\0';

    esp_netif_ip_info_t ip_info;
    esp_netif_get_ip_info(esp_netif_get_handle_from_ifkey("WIFI_STA_DEF"), &ip_info);
    snprintf(device_parameter->ip_address, sizeof(device_parameter->ip_address), IPSTR, IP2STR(&ip_info.ip));

    wifi_ap_record_t ap;
    esp_wifi_sta_get_ap_info(&ap);
    snprintf(device_parameter->mac, sizeof(device_parameter->mac), "%02X:%02X:%02X:%02X:%02X:%02X", ap.bssid[0], ap.bssid[1], ap.bssid[2], ap.bssid[3], ap.bssid[4], ap.bssid[5]);

    device_parameter->bootnum = g_restart_counter;

    strncpy(device_parameter->firmware_version, FIRMWARE_VERSION_STRING, sizeof(device_parameter->firmware_version) - 1);
    device_parameter->firmware_version[sizeof(device_parameter->firmware_version) - 1] = '\0'; // Ensure null termination

}
void get_device_frequent_parameter(device_parameter_t *device_parameter)
{
    device_parameter->uptime = esp_timer_get_time() / 3600000000.0;

    wifi_ap_record_t ap;
    esp_wifi_sta_get_ap_info(&ap);
    device_parameter->rssi = ap.rssi;

}

void temp_cpu_init(uint8_t temp_min, uint8_t temp_max)
{
    g_cpu_temp_sensor =NULL;
    temperature_sensor_config_t temp_sensor_config =
    {
        .range_min = temp_min,
        .range_max = temp_max,
    };
    temperature_sensor_install(&temp_sensor_config, &g_cpu_temp_sensor);
    ESP_ERROR_CHECK(temperature_sensor_enable(g_cpu_temp_sensor));
}
void get_cpu_temp(float *cpu_temp)
{
    temperature_sensor_get_celsius(g_cpu_temp_sensor, cpu_temp);
    printf("CPU Temperature: %.02f °C\n",*cpu_temp);
}
esp_err_t power_save(void)
{
    // Configure dynamic frequency scaling:
    // maximum and minimum frequencies are set in sdkconfig,
    // automatic light sleep is enabled if tickless idle support is enabled.
    esp_wifi_set_ps(WIFI_PS_MIN_MODEM);
    esp_pm_config_t pm_config = {
            .max_freq_mhz = 80,
            .min_freq_mhz = 10,
    #if CONFIG_FREERTOS_USE_TICKLESS_IDLE
            .light_sleep_enable = false
    #endif
    };
    return esp_pm_configure(&pm_config);
}
int id_to_port(char *client_id)
{
    return (5000 + atoi(strtok(client_id,"envisor_testing_")));
}