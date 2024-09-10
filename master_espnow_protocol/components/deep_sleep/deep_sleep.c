#include "deep_sleep.h"

// void deep_sleep_register_gpio_wakeup()
// {
//     const gpio_config_t config = 
//     {
//         .pin_bit_mask = BIT(WAKEUP_GPIO_PIN),
//         .mode = GPIO_MODE_INPUT,
//     };

//     ESP_ERROR_CHECK(gpio_config(&config));
//     ESP_ERROR_CHECK(esp_deep_sleep_enable_gpio_wakeup(BIT(WAKEUP_GPIO_PIN), ESP_GPIO_WAKEUP_GPIO_HIGH));

//     printf("Enabling GPIO wakeup on pins GPIO%d\n", WAKEUP_GPIO_PIN);
// }

void deep_sleep_register_rtc_timer_wakeup()
{
    const int wakeup_time_sec = ENABLE_TIMER_WAKEUP;
    printf("Enabling timer wakeup, %ds\n", wakeup_time_sec);
    ESP_ERROR_CHECK(esp_sleep_enable_timer_wakeup(wakeup_time_sec * 1000000));
}

void deep_sleep_mode()
{   
    printf("Entering deep sleep\n");

    /* Get timestamp before entering sleep */
    int64_t t_before_us = esp_timer_get_time();

    /* Enter sleep mode */
    esp_deep_sleep_start();

    /* Get timestamp after waking up from sleep */
    int64_t t_after_us = esp_timer_get_time();

    /* Determine wake up reason */
    const char* wakeup_reason = "timer";

    printf("Returned from deep sleep, reason: %s, t=%lld ms, slept for %lld ms\n",
        wakeup_reason, t_after_us / 1000, (t_after_us - t_before_us) / 1000);

}
