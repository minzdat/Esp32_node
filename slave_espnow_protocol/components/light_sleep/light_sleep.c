#include "light_sleep.h"

int64_t start_time_light_sleep = 0;

void processing_before_lightsleep(void) 
{
    // Processing before light sleep
    if (slave_espnow_handle != NULL) 
    {
        vTaskSuspend(slave_espnow_handle);
        ESP_LOGI(TAG_LIGHT_SLEEP, "Suspended slave_espnow_task");
    }

    esp_err_t ret;
    ret = esp_now_unregister_send_cb();
    if (ret == ESP_OK) {
        ESP_LOGI("ESP-NOW", "Unregister send callback successfully.");
    } else {
        ESP_LOGE("ESP-NOW", "Failed to unregister send callback: %s", esp_err_to_name(ret));
    }
    ret = esp_now_unregister_recv_cb();
    if (ret == ESP_OK) {
        ESP_LOGI("ESP-NOW", "Unregister receive callback successfully.");
    } else {
        ESP_LOGE("ESP-NOW", "Failed to unregister receive callback: %s", esp_err_to_name(ret));
    }
    slave_espnow_deinit();      
}

void processing_after_lightsleep(void) 
{
    slave_espnow_init(); 
    if (slave_espnow_handle != NULL) 
    {
        vTaskResume(slave_espnow_handle);
        ESP_LOGI(TAG_LIGHT_SLEEP, "Resumed slave_espnow_task");
    }
}

void light_sleep_task(void *args)
{
    while (true) 
    {
        ESP_LOGE(TAG_LIGHT_SLEEP, "Task light_sleep_task");
        if (s_master_unicast_mac.connected)
        {
            EventBits_t uxBits = xEventGroupWaitBits(xEventGroupLightSleep, LIGHT_SLEEP_BIT, pdFALSE, pdFALSE, portMAX_DELAY);
            if (uxBits & LIGHT_SLEEP_BIT) 
            {
                int64_t current_time = esp_timer_get_time();
                if ((current_time - start_time_light_sleep) >= TIMER_LIGHT_SLEEP) 
                {
                    ESP_LOGI(TAG_LIGHT_SLEEP, "Timer wake up : %lld us", (current_time - start_time_light_sleep));

                    printf("Entering light sleep\n");
                    /* To make sure the complete line is printed before entering sleep mode,
                    * need to wait until UART TX FIFO is empty:
                    */
                    uart_wait_tx_idle_polling(CONFIG_ESP_CONSOLE_UART_NUM);
                
                    /* Get timestamp before entering sleep */
                    int64_t t_before_us = esp_timer_get_time();
                    
                    /* Enter sleep mode */
                    esp_light_sleep_start();

                    /* Get timestamp after waking up from sleep */
                    int64_t t_after_us = esp_timer_get_time();

                    // Processing after light sleep
                    start_time_light_sleep = t_after_us;

                    /* Determine wake up reason */
                    const char* wakeup_reason;
                    switch (esp_sleep_get_wakeup_cause()) 
                    {
                        case ESP_SLEEP_WAKEUP_TIMER:
                            wakeup_reason = "timer";
                            break;
                        case ESP_SLEEP_WAKEUP_GPIO:
                            wakeup_reason = "pin";
                            break;
                        case ESP_SLEEP_WAKEUP_UART:
                            wakeup_reason = "uart";
                            /* Hang-up for a while to switch and execute the uart task
                            * Otherwise the chip may fall sleep again before running uart task */
                            vTaskDelay(1);
                            break;
                        #if TOUCH_LSLEEP_WAKEUP_SUPPORT
                            case ESP_SLEEP_WAKEUP_TOUCHPAD:
                                wakeup_reason = "touch";
                                break;
                        #endif
                            default:
                                wakeup_reason = "other";
                                break;
                    }
                    #if CONFIG_NEWLIB_NANO_FORMAT
                        /* printf in newlib-nano does not support %ll format, causing test fail */
                        printf("Returned from light sleep, reason: %s, t=%d ms, slept for %d ms\n",
                            wakeup_reason, (int) (t_after_us / 1000), (int) ((t_after_us - t_before_us) / 1000));
                    #else
                        printf("Returned from light sleep, reason: %s, t=%lld ms, slept for %lld ms\n",
                            wakeup_reason, t_after_us / 1000, (t_after_us - t_before_us) / 1000);
                    #endif
                    if (esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_GPIO) 
                    {
                        /* Waiting for the gpio inactive, or the chip will continuously trigger wakeup*/
                        wait_gpio_inactive();
                    }
                }
            }

        }             
        vTaskDelay(pdMS_TO_TICKS(100));   
    }
    vTaskDelete(NULL);
}

void light_sleep_init()
{
    /* Enable wakeup from light sleep by timer */
    register_timer_wakeup();
}
