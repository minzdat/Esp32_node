#include "slave_controller.h"

bool relay_state = false;

void relay_init(void)
{
    gpio_config_t io_conf;
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = (1ULL << GPIO_RELAY_PIN);
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    gpio_config(&io_conf);

    rtc_gpio_init(GPIO_RELAY_PIN);
    rtc_gpio_set_direction(GPIO_RELAY_PIN, RTC_GPIO_MODE_OUTPUT_ONLY);
    rtc_gpio_set_level(GPIO_RELAY_PIN, gpio_get_level(GPIO_RELAY_PIN));

    gpio_set_level(GPIO_RELAY_PIN, 1);
    rtc_gpio_set_level(GPIO_RELAY_PIN, 1);
}

void relay_on(void)
{
    gpio_set_level(GPIO_RELAY_PIN, 0);
    rtc_gpio_set_level(GPIO_RELAY_PIN, 0);
    relay_state = true;
}

void relay_off(void)
{
    gpio_set_level(GPIO_RELAY_PIN, 1);
    rtc_gpio_set_level(GPIO_RELAY_PIN, 1);
    relay_state = false;
}

void led_init(void)
{
    gpio_config_t io_conf;
    
    // LED CONNECT configuration
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = (1ULL << GPIO_LED_CONNECT);
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    gpio_config(&io_conf);

    rtc_gpio_init(GPIO_LED_CONNECT);
    rtc_gpio_set_direction(GPIO_LED_CONNECT, RTC_GPIO_MODE_OUTPUT_ONLY);
    rtc_gpio_set_level(GPIO_LED_CONNECT, gpio_get_level(GPIO_LED_CONNECT));
    
    // LED configuration
    io_conf.pin_bit_mask = (1ULL << GPIO_LED);
    gpio_config(&io_conf);

    rtc_gpio_init(GPIO_LED);
    rtc_gpio_set_direction(GPIO_LED, RTC_GPIO_MODE_OUTPUT_ONLY);
    rtc_gpio_set_level(GPIO_LED, gpio_get_level(GPIO_LED));
}

void led_control(led_id_t led_id, bool state)
{
    // Turn LED on or off based on id
    switch (led_id)
    {
        case LED_CONNECT:

            gpio_set_level(GPIO_LED_CONNECT, state);
            rtc_gpio_set_level(GPIO_LED_CONNECT, state);

            ESP_LOGI(TAG_SLAVE_CONTROLLER, "LED CONNECT %s", state ? "ON" : "OFF");

            break;

        case LED:

            gpio_set_level(GPIO_LED, state);
            rtc_gpio_set_level(GPIO_LED, state);

            ESP_LOGI(TAG_SLAVE_CONTROLLER, "LED PIN %s", state ? "ON" : "OFF");

            break;

        default:

            ESP_LOGW(TAG_SLAVE_CONTROLLER, "Invalid LED ID");

            break;
    }
}


void handle_device(device_type_t device_type, bool state) 
{
    switch (device_type) 
    {
        case DEVICE_RELAY:
            // Controller Relay

            ESP_LOGI(TAG_SLAVE_CONTROLLER, "Processing RELAY");
            
            if (relay_state) 
            {
                relay_off();  
                ESP_LOGI(TAG_SLAVE_CONTROLLER, "RELAY OFF");
            } 
            else 
            {
                relay_on(); 
                ESP_LOGI(TAG_SLAVE_CONTROLLER, "RELAY ON");
            }            

            break;
        
        case DEVICE_LED_CONNECT:
            // Controller LED CONNECT

            ESP_LOGI(TAG_SLAVE_CONTROLLER, "Processing LED CONNECT");

            led_control(LED_CONNECT, state);

            break;

        case DEVICE_LED:
            // Controller LED

            ESP_LOGI(TAG_SLAVE_CONTROLLER, "Processing LED");

            break;

        case DEVICE_FLOAT:
            // Controller FLOAT

            ESP_LOGI(TAG_SLAVE_CONTROLLER, "Processing Float");

            break;

        case DISCONNECT_NODE:
            // Disconnect node

            ESP_LOGI(TAG_SLAVE_CONTROLLER, "Processing DISCONNECT");
            
            s_master_unicast_mac.connected = false;
            s_master_unicast_mac.count_keep_connect = 0;
            save_to_nvs(NVS_KEY_CONNECTED, NVS_KEY_KEEP_CONNECT, NVS_KEY_PEER_ADDR, s_master_unicast_mac.connected, s_master_unicast_mac.count_keep_connect, s_master_unicast_mac.peer_addr);
            // Off LED CONNECT
            handle_device(DEVICE_LED_CONNECT, s_master_unicast_mac.connected);

            break;

        default:
            // Unspecified case

            ESP_LOGW(TAG_SLAVE_CONTROLLER, "Invalid Device Type");

            break;
    }
}


void slave_controller_init()
{
    relay_init();
    led_init();
}