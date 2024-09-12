idf_component_register(SRCS "global.c"
                    INCLUDE_DIRS "include"
                    REQUIRES driver json mqtt esp_wifi nvs_flash app_update esp_http_client esp_https_ota mbedtls esp_system main)