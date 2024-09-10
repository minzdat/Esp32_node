idf_component_register(SRCS "read_temp.c" "slave_espnow_protocol.c" "wifi_espnow.c"
                    INCLUDE_DIRS "include" REQUIRES nvs_flash esp_event esp_netif esp_wifi esp_timer driver read_serial json)
