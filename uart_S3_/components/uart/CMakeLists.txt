idf_component_register(SRCS "uart.c"
                    INCLUDE_DIRS "include"
                    REQUIRES esp_timer driver json PubSubClient)