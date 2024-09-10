<<<<<<< HEAD
idf_component_register(SRCS "nvs_espnow.c" "read_temp.c" "slave_espnow_protocol.c" "wifi_espnow.c"
                    INCLUDE_DIRS "include"
                    REQUIRES nvs_flash esp_event esp_netif esp_wifi esp_timer driver deep_sleep light_sleep)
=======
idf_component_register(SRCS "read_temp.c" "slave_espnow_protocol.c" "wifi_espnow.c"
                    INCLUDE_DIRS "include" REQUIRES nvs_flash esp_event esp_netif esp_wifi esp_timer driver sleep)
>>>>>>> 0d65c9acca272f1193113c0af4e2c3e13a3f601f
