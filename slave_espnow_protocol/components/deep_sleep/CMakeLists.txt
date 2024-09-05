idf_component_register(SRCS "deep_sleep.c" "ext_wakeup.c" "gpio_wakeup.c" "touch_wakeup.c"
                    INCLUDE_DIRS "include" 
                    REQUIRES esp_timer driver json nvs_flash)
