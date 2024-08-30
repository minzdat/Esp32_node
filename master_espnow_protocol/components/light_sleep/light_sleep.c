#include "light_sleep.h"

void light_sleep_mode()
{
    printf("Entering light sleep\n");

    /* Get timestamp before entering sleep */
    int64_t t_before_us = esp_timer_get_time();

    /* Enter sleep mode */
    esp_light_sleep_start();

    /* Get timestamp after waking up from sleep */
    int64_t t_after_us = esp_timer_get_time();

    /* Determine wake up reason */
    const char* wakeup_reason = "timer";

    printf("Returned from light sleep, reason: %s, t=%lld ms, slept for %lld ms\n",
        wakeup_reason, t_after_us / 1000, (t_after_us - t_before_us) / 1000);

    // master_espnow_deinit();
    // master_espnow_init();
}