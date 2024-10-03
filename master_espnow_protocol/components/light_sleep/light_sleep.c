#include "light_sleep.h"

uint32_t all_slaves_bits = 0;
int64_t sleep_duration = 0;
int64_t timer_wakeup = TIMER_WAKEUP_TIME_US;
static int64_t t_before_wakeup;
static int64_t t_after_wakeup;

void processing_before_lightsleep(void) 
{
    /* Enter sleep mode */
    vTaskDelay(pdMS_TO_TICKS(500));  

    // Pause espnow's tasks 
    if (master_espnow_handle != NULL) 
    {
        vTaskSuspend(master_espnow_handle);
        ESP_LOGI(TAG_LIGHT_SLEEP, "Suspended master_espnow_task");
    }
    if (retry_connect_lost_handle != NULL) 
    {
        vTaskSuspend(retry_connect_lost_handle);
        ESP_LOGI(TAG_LIGHT_SLEEP, "Suspended retry_connect_lost_task");
    } 

    // Processing before light sleep
    #if CONFIG_ESPNOW_WITH_WIFI 
        esp_wifi_stop(); // Optionally stop WiFi
    #endif //CONFIG_ESPNOW_WITH_WIFI
    
    // Deinit espnow before light sleep
    for (int i = 0; i < MAX_SLAVES; i++) 
    {
        if (allowed_connect_slaves[i].status) // Check online status
        {
            erase_peer(allowed_connect_slaves[i].peer_addr);
        }
    }

    ESP_ERROR_CHECK( esp_now_set_pmk((uint8_t *)"") ); // PMK rỗng

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
    esp_now_deinit();                 
}

void processing_after_lightsleep(void) 
{
    #if CONFIG_ESPNOW_WITH_WIFI
        // Ensure WiFi is running after wake up
        esp_wifi_start(); // Optionally start WiFi
        wait_for_wifi_connection(); // Ensure WiFi is connected
    #endif //CONFIG_ESPNOW_WITH_WIFI

    // Init espnow after light sleep
    master_espnow_init();

    // OFF light sleep
    for (int i = 0; i < MAX_SLAVES; i++) 
    {
        if (allowed_connect_slaves[i].status) // Check online status
        {
            add_peer(allowed_connect_slaves[i].peer_addr, true);
            // Check recv response STILL_CONNECT
            if (allowed_connect_slaves[i].check_keep_connect)
            {
                xQueueSend(slave_disconnect_queue, &i, portMAX_DELAY);
            }
            allowed_connect_slaves[i].check_connect_success = false;
        }
    }

    // Continue espnow's tasks
    if (master_espnow_handle != NULL) 
    {
        vTaskResume(master_espnow_handle);
        ESP_LOGI(TAG_LIGHT_SLEEP, "Resumed master_espnow_task");
    }
    if (retry_connect_lost_handle != NULL) 
    {
        vTaskResume(retry_connect_lost_handle);
        ESP_LOGI(TAG_LIGHT_SLEEP, "Resumed retry_connect_lost_task");
    }
}

void light_sleep_task(void *args)
{
    while (true) 
    {
        if (devices_online > 0)
        {
            EventBits_t uxBits = xEventGroupWaitBits(xEventGroupLightSleep, all_slaves_bits, pdFALSE, pdTRUE, pdMS_TO_TICKS(100));
            if ((uxBits & all_slaves_bits) == all_slaves_bits) 
            {
                xEventGroupClearBits(xEventGroupLightSleep, all_slaves_bits);

                printf("Entering light sleep\n");
                /* To make sure the complete line is printed before entering sleep mode,
                * need to wait until UART TX FIFO is empty:
                */
                uart_wait_tx_idle_polling(CONFIG_ESP_CONSOLE_UART_NUM);

                processing_before_lightsleep();
                
                /* Get timestamp before entering sleep */
                int64_t t_before_us = esp_timer_get_time();
                t_after_wakeup = t_before_us;

                printf("Timer wake up for %lld ms\n", (t_after_wakeup - t_before_wakeup) / 1000);

                esp_light_sleep_start();

                /* Get timestamp after waking up from sleep */
                int64_t t_after_us = esp_timer_get_time();
                t_before_wakeup = t_after_us;

                /* Determine wake up reason */
                const char* wakeup_reason;
                switch (esp_sleep_get_wakeup_cause()) 
                {
                    case ESP_SLEEP_WAKEUP_TIMER:
                        wakeup_reason = "timer";

                        sleep_duration = 0;
                        timer_wakeup = TIMER_WAKEUP_TIME_US - sleep_duration;
                        ESP_LOGI(TAG_LIGHT_SLEEP, "Set up timer wakeup: %lld us", timer_wakeup);
                        register_timer_wakeup(timer_wakeup);

                        break;
                    case ESP_SLEEP_WAKEUP_GPIO:
                        wakeup_reason = "pin";

                        sleep_duration = (t_after_us - t_before_us);
                        ESP_LOGI(TAG_LIGHT_SLEEP, "Slept for: %lld us", sleep_duration);
                        timer_wakeup = TIMER_WAKEUP_TIME_US - sleep_duration;
                        ESP_LOGI(TAG_LIGHT_SLEEP, "Set up timer wakeup: %lld us", timer_wakeup);
                        register_timer_wakeup(timer_wakeup);

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

                    // Response wakeup uart to S3
                    dump_uart((uint8_t *)WOKE_UP,sizeof(WOKE_UP));
                }

                processing_after_lightsleep(); 
            }
        }
        vTaskDelay(pdMS_TO_TICKS(500));   
    }
    vTaskDelete(NULL);
}

void light_sleep_init()
{
    // Enable wakeup from light sleep by timer
    register_timer_wakeup(timer_wakeup);
    // Enable wakeup from light sleep by gpio
    // register_gpio_wakeup();
}
