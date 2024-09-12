#include "light_sleep.h"

esp_err_t register_timer_wakeup(uint64_t timer_wakeup)
{
    ESP_RETURN_ON_ERROR(esp_sleep_enable_timer_wakeup(timer_wakeup), TAG_LIGHT_SLEEP, "Configure timer as wakeup source failed");
    ESP_LOGI(TAG_LIGHT_SLEEP, "timer wakeup source is ready");
    return ESP_OK;
}