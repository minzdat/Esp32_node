idf_component_register(SRCS "master_espnow_protocol.c" "nvs_espnow.c" "read_temp.c" "wifi_espnow.c"
                    INCLUDE_DIRS "include" REQUIRES nvs_flash esp_event esp_netif esp_wifi esp_http_client esp_timer driver read_serial mbedtls)
