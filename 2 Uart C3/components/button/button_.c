#include "button_.h"
static const char *TAG = "UART";
static void button_single_cb(void *arg, void *usr_data)
{
    static bool status = 0;
    ESP_ERROR_CHECK(!(BUTTON_SINGLE_CLICK == iot_button_get_event(arg)));
    ESP_LOGI(TAG, "single");
    
}

static void button_double_cb(void *arg, void *usr_data)
{
    ESP_ERROR_CHECK(!(BUTTON_DOUBLE_CLICK == iot_button_get_event(arg)));
    ESP_LOGI(TAG, "double");
}

static void button_longpress_cb(void *arg, void *usr_data)
{
    ESP_ERROR_CHECK(!(BUTTON_LONG_PRESS_START == iot_button_get_event(arg)));
    ESP_LOGI(TAG, "long press");
}
void button_init(){
    // create gpio button
    button_config_t button_config = {
        .type = BUTTON_TYPE_GPIO,
        .long_press_time = CONFIG_BUTTON_LONG_PRESS_TIME_MS,
        .short_press_time = CONFIG_BUTTON_SHORT_PRESS_TIME_MS,
        .gpio_button_config = {
            .gpio_num = CONTROL_KEY_GPIO,
            .active_level = 0,
        },
    };
    button_handle_t gpio_btn = iot_button_create(&button_config);
    if(NULL == gpio_btn) {
        ESP_LOGE(TAG, "Button create failed");
    }
    button_handle_t button_handle = iot_button_create(&button_config);
    iot_button_register_cb(button_handle, BUTTON_SINGLE_CLICK, button_single_cb, NULL);
    iot_button_register_cb(button_handle, BUTTON_DOUBLE_CLICK, button_double_cb, NULL);
    iot_button_register_cb(button_handle, BUTTON_LONG_PRESS_START, button_longpress_cb, NULL);
}