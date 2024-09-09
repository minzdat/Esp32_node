
idf_component_register(
    SRCS "src/connect_wifi.c"
    INCLUDE_DIRS "include"
    REQUIRES esp_wifi esp_event nvs_flash esp_http_client mbedtls esp_timer 

)
