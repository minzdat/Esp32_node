<<<<<<< HEAD
idf_component_register( SRCS "master_espnow_protocol.c" "nvs_espnow.c" "read_temp.c" "wifi_espnow.c"
                        INCLUDE_DIRS "include" 
                        REQUIRES nvs_flash esp_event esp_netif esp_wifi esp_http_client esp_timer driver esp_pm deep_sleep light_sleep read_serial)
=======
idf_component_register(SRCS "master_espnow_protocol.c" "nvs_espnow.c" "read_temp.c" "wifi_espnow.c"
                    INCLUDE_DIRS "include" REQUIRES nvs_flash esp_event esp_netif esp_wifi esp_http_client esp_timer driver uart sleep)
>>>>>>> 0d65c9acca272f1193113c0af4e2c3e13a3f601f
