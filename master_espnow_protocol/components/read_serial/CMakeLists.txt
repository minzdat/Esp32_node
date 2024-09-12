idf_component_register(SRCS "read_serial.c"
                    INCLUDE_DIRS "include"
                    REQUIRES esp_timer driver json mbedtls esp_wifi master_controller)
