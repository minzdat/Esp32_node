#include "light_sleep.h"

bool light_sleep_flag = false;
int64_t start_time_light_sleep = 0;
int64_t sleep_duration = 0;
int64_t timer_wakeup = TIMER_WAKEUP_TIME_US;

void light_sleep_task(void *args)
{
    while (true) 
    {
        ESP_LOGE(TAG_LIGHT_SLEEP, "Task light_sleep_task");

        if (light_sleep_flag)
        {
            int64_t current_time = esp_timer_get_time();

            if ((current_time - start_time_light_sleep) >= TIMER_LIGHT_SLEEP) 
            {

                ESP_LOGI(TAG_LIGHT_SLEEP, "Current time light sleep : %lld us", current_time);
                ESP_LOGI(TAG_LIGHT_SLEEP, "Start time light sleep   : %lld us", start_time_light_sleep);
                ESP_LOGI(TAG_LIGHT_SLEEP, "Enter time light sleep   : %lld us", (current_time - start_time_light_sleep));

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
                }

                master_espnow_deinit();                
                master_espnow_init(); 

                if (esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_GPIO) 
                {
                    dump_uart((uint8_t *)WOKE_UP,sizeof(WOKE_UP));
                    //handle test control device
                    // handle_device(DISCONNECT_NODE, true); 
                }
                
                start_time_light_sleep = 0;
                light_sleep_flag = false;   
            
            }
                    
        }

        vTaskDelay(pdMS_TO_TICKS(1000));   
    }
    vTaskDelete(NULL);
}

void light_sleep_init()
{
    // Enable wakeup from light sleep by timer
    register_timer_wakeup(timer_wakeup);
    // Enable wakeup from light sleep by gpio
    register_gpio_wakeup();
}
