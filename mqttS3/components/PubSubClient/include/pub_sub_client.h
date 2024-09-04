// wifi_init_sta();
// mqtt_init("mqtt://35.240.204.122:1883", "tts-2", NULL);
// subcribe_to_topic("v1/devices/me/rpc/request/+",2);

// sprintf(data, "temperature_rdo: %f, do: %f, temperature_phg: %f, ph: 0",sin_angle,sin_angle2,sin_angle3);
// data_to_mqtt(data, "v1/devices/me/telemetry",500, 1);


#ifndef PUB_SUBCLIENT_H
#define PUB_SUBCLIENT_H

#include "mqtt_client.h"
#include "cJSON.h"
#include "string.h"
#include "esp_log.h"




// void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data);
// void parse_and_add_to_json(const char *input_string, cJSON *json_obj);
void data_to_mqtt(char *data, char *topic,int delay_time_ms,int qos);
void mqtt_init(char *broker_uri, char *username, char *client_id);
void subcribe_to_topic(char *topic,int qos);
void get_data_subcribe_topic( uint16_t *data);
void mqtt_subcriber(esp_mqtt_event_handle_t event);
#endif // PUB_SUBCLIENT
