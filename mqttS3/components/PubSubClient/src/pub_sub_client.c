#include "pub_sub_client.h"

const char *MQTT_TAG = "MQTT";
esp_mqtt_client_handle_t g_mqtt_client;  
char g_buffer_sub[10];
static EventGroupHandle_t g_mqtt_event_group;
const int g_constant_ConnectBit = BIT0;
const int g_constant_PublishedBit = BIT1;
double g_salinity_value=0;

/**
 * @brief Handles MQTT events.
 *
 * This function is the event handler for MQTT-related events. It processes different
 * event types such as connection, disconnection, subscription, unsubscription, published data,
 * and error events. Depending on the event type, appropriate actions are taken (e.g., logging,
 * setting event group bits, parsing received data).
 *
 * @param[in] handler_args Unused (can be NULL).
 * @param[in] base The event base (unused in this context).
 * @param[in] event_id The MQTT event ID.
 * @param[in] event_data The event data containing relevant information.
 *
 * @note Make sure the MQTT client is properly initialized and started before using this handler.
 */

void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    esp_mqtt_event_handle_t event = event_data;
    switch ((esp_mqtt_event_id_t)event->event_id)
    {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(MQTT_TAG, "MQTT_EVENT_CONNECTED");
            xEventGroupSetBits(g_mqtt_event_group,g_constant_ConnectBit);
            // esp_mqtt_client_subscribe(g_mqtt_event_group,"v1/devices/me/rpc/request/+",0);
            break;
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(MQTT_TAG, "MQTT_EVENT_DISCONNECTED");
            xEventGroupClearBits(g_mqtt_event_group,g_constant_ConnectBit);
            break;
        case MQTT_EVENT_SUBSCRIBED:
            ESP_LOGI(MQTT_TAG, "MQTT_EVENT_SUBSCRIBED");
            break;
        case MQTT_EVENT_UNSUBSCRIBED:
            ESP_LOGI(MQTT_TAG, "MQTT_EVENT_UNSUBSCRIBED");
            break;
        case MQTT_EVENT_PUBLISHED:
            xEventGroupSetBits(g_mqtt_event_group,g_constant_PublishedBit);
            ESP_LOGI(MQTT_TAG, "MQTT_EVENT_PUBLISHED");
            break;
        case MQTT_EVENT_DATA:
            ESP_LOGI(MQTT_TAG, "MQTT_RECEIVED DATA");
            //memcpy(g_buffer_sub, event->data, event->data_len);
            mqtt_subcriber(event);
            break;
        case MQTT_EVENT_ERROR:
            break;
        default:
            ESP_LOGI(MQTT_TAG, "Other event id:%d", event->event_id);
            break;
    }
}
/**
 * @brief Initializes and starts the MQTT client.
 *
 * This function sets up the MQTT client with the provided broker URI and username.
 * It creates an event group, initializes the client configuration, registers an event handler,
 * and starts the MQTT client.
 *
 * @param[in] broker_uri The MQTT broker URI.
 * @param[in] username The username for authentication (if required).
 *
 * @note Make sure to call this function after configuring the MQTT parameters.
 */
void mqtt_init(char *broker_uri, char *username, char *client_id)
{
    g_mqtt_event_group = xEventGroupCreate();
    esp_mqtt_client_config_t mqtt_cfg = 
    {
        .broker.address.uri = broker_uri,  // MQTT broker URI from configuration
        .credentials.username = username,
        .credentials.client_id = client_id,
        .session.keepalive = 30
    };
    g_mqtt_client = esp_mqtt_client_init(&mqtt_cfg);  // Initialize the MQTT client
    esp_mqtt_client_register_event(g_mqtt_client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);  // Register the event handler
    esp_mqtt_client_start(g_mqtt_client);
}
/**
 * @brief Parses an input string and adds key-value pairs to a JSON object.
 *
 * This function tokenizes the input string using delimiters (": " and ", ") to extract
 * key-value pairs. It converts the value strings to double and adds them to the provided
 * cJSON object. If memory allocation fails, an error message is printed.
 *
 * @param[in] input_string The input string containing key-value pairs.
 * @param[in,out] json_obj The cJSON object to which key-value pairs are added.
 *
 * @note Make sure the cJSON library is initialized before calling this function.
 */
void parse_and_add_to_json(const char *input_string, cJSON *json_obj) 
{
    char *input_copy = strdup(input_string);
    if (input_copy == NULL) 
    {
        printf("Failed to allocate memory.\n");
        return;
    }
    char * obj;
    char * val;
    obj = strtok(input_copy, ": ");
    val = strtok(NULL, ", ");
    cJSON_AddNumberToObject(json_obj, obj, strtod(val,NULL));
    while(obj!=NULL && val!=NULL)
    {
        obj = strtok(NULL, ": ");
        val = strtok(NULL, ", ");
        if(obj!=NULL && val!=NULL)
        {
            cJSON_AddNumberToObject(json_obj, obj, strtod(val,NULL));
        }
    }    // Free the copy of input string
    free(input_copy);
}

void mqtt_subcriber(esp_mqtt_event_handle_t event)
{
    char send[100];
    strncpy(send,event->data,event->data_len);
    cJSON* data_sub=cJSON_Parse(send);
    cJSON *params=cJSON_GetObjectItem(data_sub,"params");
    char *my_json_string = cJSON_Print(params);
    g_salinity_value=cJSON_GetObjectItem(params,"salinity")->valuedouble;
    cJSON_Delete(data_sub);
    cJSON_free(my_json_string);

}

/**
 * @brief Publishes data to an MQTT topic.
 *
 * This function publishes the provided data to the specified MQTT topic using the configured MQTT client.
 * It creates a JSON object from the data, converts it to a string, and publishes it.
 * If the publishing fails, appropriate log messages are generated.
 *
 * @param[in] data The data to be published.
 * @param[in] topic The MQTT topic to publish to.
 * @param[in] delay_time_ms The delay time (in milliseconds) after publishing.
 * @param[in] qos The desired quality of service (0, 1, or 2).
 *
 * @note Make sure the MQTT client is connected before calling this function.
 */
void data_to_mqtt(char *data, char *topic, int delay_time_ms, int qos)
{
    xEventGroupWaitBits(g_mqtt_event_group,g_constant_ConnectBit,false,true,portMAX_DELAY);
    int len = strlen(data);
    if (len > 0) 
    {
        data[len] = '\0';  // Null-terminate the received data
        // Publish the data to the MQTT broker
        cJSON *json_obj = cJSON_CreateObject();
        if (json_obj == NULL) 
        {
            printf("Failed to create JSON object.\n");
            return;
        }
        parse_and_add_to_json(data,json_obj);
        char *json_data = cJSON_Print(json_obj);
   

        int ret=esp_mqtt_client_publish(g_mqtt_client, topic, json_data, strlen(json_data), qos, 0);
        if (ret == -1)
        {
            ESP_LOGE(MQTT_TAG, "Failed to publish data!");
            // cJSON_Delete(json_obj);
            // free(json_data);
            // return 1;
        }
        else if (ret == -2)
        {
            ESP_LOGW(MQTT_TAG, "Data buffer full!");
            // cJSON_Delete(json_obj);
            // free(json_data);
            // return 1;
        }
        else
        {
            printf("Message sent: %s \n", json_data);
            // cJSON_Delete(json_obj);
            // free(json_data);
        }
        cJSON_Delete(json_obj);
        free(json_data);
    }
    vTaskDelay(delay_time_ms/portTICK_PERIOD_MS);
    xEventGroupWaitBits(g_mqtt_event_group,g_constant_PublishedBit,true,true,portMAX_DELAY);
}
/**
 * @brief Subscribes to an MQTT topic with the specified quality of service (QoS).
 *
 * This function subscribes to an MQTT topic using the provided client and topic name.
 * If the subscription fails, an error message is logged.
 *
 * @param[in] topic The topic to subscribe to.
 * @param[in] qos The desired quality of service (0, 1, or 2).
 *
 * @note Make sure the MQTT client is connected before calling this function.
 */
void subcribe_to_topic(char *topic,int qos)
{
    xEventGroupWaitBits(g_mqtt_event_group,g_constant_ConnectBit,false,true,portMAX_DELAY);
    if(esp_mqtt_client_subscribe(g_mqtt_client,topic,qos) == -1) 
    {
        ESP_LOGE(MQTT_TAG,"Failed to subcribe to topic");
    }
}
void get_data_subcribe_topic( uint16_t *data)
{
    *data=(uint16_t)(g_salinity_value*10);
}
