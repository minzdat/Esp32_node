idf_component_register(SRCS "slave_espnow_protocol.c" "wifi_espnow.c"
                    INCLUDE_DIRS "include" REQUIRES nvs_flash esp_event esp_netif esp_wifi esp_timer)
