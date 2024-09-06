#include <stdio.h>
#include "sleep.h"
#include "driver/uart.h"

static const char *TAG = "timer_wakeup";

/**
 * @brief Registers the timer as a wakeup source.
 *
 * This function configures the timer to wake up the system after a specified sleep time.
 *
 * @param[in] timer_sleep_in_s Sleep time in seconds.
 * @return ESP_OK if successful, or an error code if the configuration fails.
 */
// esp_err_t register_timer_wakeup(int timer_sleep_in_s)
// {
//     int sleep_time = timer_sleep_in_s * 1000 * 1000;
//     esp_err_t err = esp_sleep_enable_timer_wakeup(sleep_time);
//     // Kiểm tra nếu có lỗi xảy ra
//     if (err != ESP_OK) {
//         ESP_LOGE(TAG, "Configure timer as wakeup source failed");
//         return err;
//     }
//     ESP_LOGI(TAG, "Timer wakeup source is ready");
//     return ESP_OK;
// }
esp_err_t register_timer_wakeup(int timer_sleep_in_s)
{
    int64_t sleep_time = (int64_t) timer_sleep_in_s * 1000 * 1000;
    esp_err_t err = esp_sleep_enable_timer_wakeup(sleep_time);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure timer as wakeup source: %s", esp_err_to_name(err));
        return err;
    }
    ESP_LOGI(TAG, "Timer wakeup source configured for %d seconds", timer_sleep_in_s);
    return ESP_OK;
}


/**
 * @brief Puts the system into light sleep mode.
 *
 * This function performs the following steps:
 * 1. Prints "Sleep!!" to the console.
 * 2. Waits for the UART transmission to complete (if applicable).
 * 3. Records the current time.
 * 4. Initiates light sleep mode.
 * 5. Calculates and prints the sleep time in milliseconds.
 */

// void go_to_sleep(void)
// {
//     printf("Sleep!!\n"); 
//     uart_wait_tx_idle_polling(CONFIG_ESP_CONSOLE_UART_NUM);  
//     int64_t time_before_sleep = esp_timer_get_time(); 
//     esp_err_t err = esp_light_sleep_start();
//     if (err != ESP_OK) {
//         ESP_LOGE(TAG, "Failed to enter light sleep: %s", esp_err_to_name(err));
//         return; 
//     }
//     int64_t time_after_sleep = esp_timer_get_time();
//     printf("Sleep time: %lld ms\n", (time_after_sleep - time_before_sleep) / 1000);
// }
void go_to_sleep(void)
{
    ESP_LOGI(TAG, "Going to sleep...");
    uart_wait_tx_idle_polling(CONFIG_ESP_CONSOLE_UART_NUM);
    int64_t time_before_sleep = esp_timer_get_time();
    esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();
    ESP_LOGI(TAG, "Wakeup cause: %d", cause);
    esp_err_t err = esp_light_sleep_start();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to enter light sleep: %s", esp_err_to_name(err));
        return;
    }
    int64_t time_after_sleep = esp_timer_get_time();
    ESP_LOGI(TAG, "System woke up, slept for: %lld ms", (time_after_sleep - time_before_sleep) / 1000);
}


/**
 * @brief Measures the running time of a given function.
 *
 * This function records the start time, executes the specified function, records the end time,
 * and calculates the time difference in milliseconds. It then prints the running time along with
 * the provided function name.
 *
 * @param[in] func Pointer to the function to be measured.
 * @param[in] func_name Name of the function (for display purposes).
 */
void calculateRunningTime(void (*func)(void), char *func_name) 
{
    // Record the start time
    //int time_before_running = esp_timer_get_time();
    
    // Execute the function
    func();
}   
    // Record the end time
    //int time_after_running = esp_timer_get_time();
    esp_err_t register_uart_wakeup(uart_port_t uart_num)
    {
        esp_err_t err = esp_sleep_enable_uart_wakeup(uart_num);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Configure UART as wakeup source failed");
            return err;
        }
        ESP_LOGI(TAG, "UART wakeup source is ready");
        return ESP_OK;
    }
    // Calculate the time difference in milliseconds
    //printf("%s running time: %d ms\n", func_name, (time_after_running - time_before_running) / 1000);

// esp_err_t register_gpio_wakeup(gpio_num_t gpio_num, int level)
// {
//     esp_err_t err = esp_sleep_enable_ext0_wakeup(gpio_num, level);
//     if (err != ESP_OK) {
//         ESP_LOGE(TAG, "Configure GPIO as wakeup source failed");
//         return err;
//     }
//     ESP_LOGI(TAG, "GPIO wakeup source is ready (GPIO%d, level: %d)", gpio_num, level);
//     return ESP_OK;
// }
void sleep_init(int sleep_time, bool uart_wakeup, uart_port_t uart_num)
{
    ESP_ERROR_CHECK(register_timer_wakeup(sleep_time));
    ESP_LOGI(TAG, "Timer wakeup registered for %d seconds", sleep_time);

    if (uart_wakeup) {
        ESP_ERROR_CHECK(register_uart_wakeup(uart_num));
        ESP_LOGI(TAG, "UART%d wakeup registered", uart_num);
    }

    // if (gpio_num >= 0) {
    //     ESP_ERROR_CHECK(register_gpio_wakeup(gpio_num, gpio_level));
    //     ESP_LOGI(TAG, "GPIO%d wakeup registered at level %d", gpio_num, gpio_level);
    // }
}

