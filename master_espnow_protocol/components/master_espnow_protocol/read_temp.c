#include "master_espnow_protocol.h"

temperature_sensor_handle_t temp_sensor = NULL;

<<<<<<< HEAD
void init_temperature_sensor()
{
=======
void init_temperature_sensor(){
>>>>>>> 0d65c9acca272f1193113c0af4e2c3e13a3f601f
    ESP_LOGI(TAG, "Read internal temperature sensor");

    // Configuring the temperature sensor
    temperature_sensor_config_t temp_sensor_config = TEMPERATURE_SENSOR_CONFIG_DEFAULT(-10, 80);
    ESP_ERROR_CHECK(temperature_sensor_install(&temp_sensor_config, &temp_sensor));

    // Enable the temperature sensor
    ESP_ERROR_CHECK(temperature_sensor_enable(temp_sensor));
}

<<<<<<< HEAD
float read_internal_temperature_sensor(void)
=======
void read_internal_temperature_sensor(void)
>>>>>>> 0d65c9acca272f1193113c0af4e2c3e13a3f601f
{
    // Read the temperature value
    float temperature_value;
    ESP_ERROR_CHECK(temperature_sensor_get_celsius(temp_sensor, &temperature_value));
<<<<<<< HEAD
    // ESP_LOGE(TAG, "Internal temperature value: %.02f C", temperature_value);

    return temperature_value;
=======
    ESP_LOGE(TAG, "Internal temperature value: %.02f C", temperature_value);
>>>>>>> 0d65c9acca272f1193113c0af4e2c3e13a3f601f
}