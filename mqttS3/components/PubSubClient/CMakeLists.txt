
idf_component_register(
    SRCS "src/pub_sub_client.c" 
    INCLUDE_DIRS "include" 
    REQUIRES esp_wifi mqtt json 
)


