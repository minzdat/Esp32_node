idf_component_register(SRCS "gpio_wakeup.c" "timer_wakeup.c" "light_sleep.c"
                    INCLUDE_DIRS "include" 
                    REQUIRES esp_timer driver master_espnow_protocol read_serial)
