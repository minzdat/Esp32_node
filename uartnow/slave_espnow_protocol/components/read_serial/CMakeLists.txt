idf_component_register(SRCS "src/read_serial.c"
                    INCLUDE_DIRS "include" 
                    REQUIRES esp_timer driver json slave_espnow_protocol mbedtls)
