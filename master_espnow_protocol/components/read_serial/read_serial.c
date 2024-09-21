#include "read_serial.h"

int time_now=0;
int time_check=0;
int timeout=10000000;
bool connect_check=true;

static QueueHandle_t uart0_queue;

void uart_config(void)
{
    uart_config_t uart_config = 
    {
        .baud_rate = BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    // uart_driver_install(UART_NUM_P2, BUF_SIZE, BUF_SIZE, 0, NULL, 0);
    uart_driver_install(UART_NUM_P2, BUF_SIZE, BUF_SIZE, 10, &uart0_queue, 0);

    // uart_driver_install(UART_NUM_P2, BUF_SIZE * 2, BUF_SIZE * 2, 20, &uart0_queue, 0);
    uart_param_config(UART_NUM_P2, &uart_config);
    // uart_set_pin(UART_NUM_P2, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    uart_set_pin(UART_NUM_P2, TX_GPIO_NUM, RX_GPIO_NUM, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    // uart0_queue = xQueueCreate(10, BUF_SIZE);

    // uart_set_pin(EX_UART_NUM_P2, TX_PIN, RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
}

void encrypt_message(const unsigned char *input, unsigned char *output, size_t length) 
{
    mbedtls_aes_context aes;
    unsigned char key[16] = "7832477891326794";
    unsigned char iv[16] =  "4892137489723148";
    
    mbedtls_aes_init(&aes);
    mbedtls_aes_setkey_enc(&aes, key, 128);
    mbedtls_aes_crypt_cbc(&aes, MBEDTLS_AES_ENCRYPT, length, iv, input, output);
    mbedtls_aes_free(&aes);
}

void dump_uart(uint8_t *message, size_t len)
{

    // len = sizeof(len);
    printf("send \n");
    uint8_t encrypted_message[len]; // AES block size = 16 bytes
    // Mã hóa tin nhắn
    encrypt_message((unsigned char *)message, encrypted_message, len);
    // uart_write_bytes(UART_NUM_P2, (const char *)message, sizeof(sensor_data_t));
    uart_write_bytes(UART_NUM_P2, (unsigned char *)message, len);
    time_check=esp_timer_get_time(); 
}

void add_json()
{
    cJSON *json_mac = cJSON_CreateObject();
    cJSON *json_data = cJSON_CreateObject();


    // Thêm các phần tử vào JSON object
    cJSON_AddStringToObject(json_mac, "Mac", "AA:BD:CC:FF:DF:FE");
    cJSON_AddNumberToObject(json_data, "temp_mcu", 20.50);
    cJSON_AddNumberToObject(json_data, "temp_do", 30.40);
    cJSON_AddItemToObject(json_mac,"Data",json_data);

    // cJSON_AddStringToObject(json_mac, "Mac", "AK:BD:CC:AB:DF:FB");
    // cJSON_AddNumberToObject(json_data, "temp_mcu", 50.20);
    // cJSON_AddNumberToObject(json_data, "temp_do", 1.50);
    // cJSON_AddItemToObject(json_mac,"Data",json_data);
    // Chuyển đổi JSON object thành chuỗi JSON
    char *json_string = cJSON_Print(json_mac);
    // Xuất ra JSON string (in ra serial)
    // printf("Size: %d \n",strlen(json_string));
    // printf("JSON: %s\n", json_string);
    uart_write_bytes(UART_NUM_P2,json_string, strlen(json_string));
}

void uart_event(void *pvParameters)
{
    uart_event_t event;
    size_t buffered_size;
    uint8_t* dtmp = (uint8_t*) malloc(BUF_SIZE);
    unsigned char decrypted_message[BUF_SIZE];
    messages_request res_getdata;
    
    while (true)
    {
        if (xQueueReceive(uart0_queue, (void *)&event, (TickType_t)portMAX_DELAY)) 
        {
            // bzero(dtmp, RD_BUF_SIZE);
            memset(decrypted_message, 0, BUF_SIZE);
            if (event.type == UART_DATA)
            {
                uart_read_bytes(UART_NUM_P2, decrypted_message, event.size, 100 / portTICK_PERIOD_MS);
                uart_flush(UART_NUM_P2);
                ESP_LOGE(TAG_READ_SERIAL, "Reicv %d bytes : ",event.size);
                printf("%s \n",decrypted_message);


                accept_connect((uint8_t *)decrypted_message);

                if (connect_check)
                {
                    if (strcmp((char *)decrypted_message, REQUEST_UART) == 0) 
                    {
                        time_now=esp_timer_get_time(); 
                        // connect_check=true;
                    }
                    else if (strcmp((char *)decrypted_message, BUTTON_MSG) == 0) 
                    {
                        time_now=esp_timer_get_time();
                        ESP_LOGE(TAG_READ_SERIAL, "Reicv %s ",decrypted_message);
                    }
                    else if ((memcmp((char *)decrypted_message, GET_DATA, 6) == 0))
                    {
                        table_device_tt sensor_data;
                        messages_request* mess_get= (messages_request*)decrypted_message;
                            // ESP_LOGE(TAG_READ_SERIAL, "size table_device_tt %d ",sizeof(table_device_tt)ư3r);
                        for (int i = 0; i < MAX_SLAVES; i++)
                        {
                            if ((memcmp(mess_get->mac, table_devices[i].peer_addr, 6)==0)&&(table_devices[i].status==1))
                            {
                                memcpy(&sensor_data,&table_devices[i], sizeof(table_device_tt));
                                ESP_LOGI("MAC Address", "MAC: %02X:%02X:%02X:%02X:%02X:%02X",
                                mess_get->mac[0], mess_get->mac[1], mess_get->mac[2], mess_get->mac[3], mess_get->mac[4], mess_get->mac[5]);
                                dump_uart((uint8_t*)&sensor_data, sizeof(table_device_tt));
                                break;
                            }
                        }
                    }
                    else if (strcmp((char *)decrypted_message,WAKE_UP_COMMAND)== 0)
                    {
                        dump_uart((uint8_t *)WOKE_UP,sizeof(WOKE_UP));
                    }
                    else if (memcmp((char *)decrypted_message, GET_FULL_DATA, 6) == 0)
                    {   
                        log_table_devices();
                        dump_uart((uint8_t*)&table_devices, sizeof(table_device_tt)*MAX_SLAVES);

                    }
                    else if (memcmp((char *)decrypted_message, SET_RELAY_STATE, 6) == 0)
                    {
                        ESP_LOGE(TAG_READ_SERIAL, "SET_RELAY_STATE");

                        messages_request* mess_relay_state= (messages_request*)decrypted_message;
                        messages_request res_relay_state;
                        ESP_LOGI("MAC Address", "MAC: %02X:%02X:%02X:%02X:%02X:%02X",
                                mess_relay_state->mac[0], mess_relay_state->mac[1], mess_relay_state->mac[2], mess_relay_state->mac[3], mess_relay_state->mac[4], mess_relay_state->mac[5]);
                        for (int i = 0; i < MAX_SLAVES; i++)
                        {
                            if ((memcmp(mess_relay_state->mac, table_devices[i].peer_addr, 6)==0))
                            {
                                ESP_LOGI("MAC Address", "MAC: %02X:%02X:%02X:%02X:%02X:%02X",
                                mess_relay_state->mac[0], mess_relay_state->mac[1], mess_relay_state->mac[2], mess_relay_state->mac[3], mess_relay_state->mac[4], mess_relay_state->mac[5]);
                                memcpy(res_relay_state.message, RESPONSE_RELAY_STATE, sizeof(RESPONSE_RELAY_STATE));
                                memcpy(res_relay_state .mac, mess_relay_state->mac, 6);
                                dump_uart((uint8_t*)&res_relay_state, sizeof(res_relay_state));
                                handle_device(DEVICE_RELAY,true, mess_relay_state->mac);
                                break;
                            }
                        }
                    }

                }
                // if ((strcmp((char *)decrypted_message, RESPONSE_AGREE) == 0)&&(!connect_check)) 
                // {
                //     connect_check=true;
                // }

            }
        }
    }
    free(dtmp);
    dtmp = NULL;
    vTaskDelete(NULL);
}

void delay(int x)
{
    vTaskDelay(pdMS_TO_TICKS(x));
}

void check_timeout()
{
    messages_request mess;
    memcpy(mess.message, REQUEST_UART, sizeof(REQUEST_UART));
    esp_wifi_get_mac(ESP_IF_WIFI_STA, mess.mac);

    while (1)
    {

        if ((time_now!=0)&&(connect_check))
        {
            time_check=esp_timer_get_time();
            printf("timenow: %d \n",time_now);
            printf("time_check: %d \n",time_check);
            if ((time_check-time_now)>timeout)
            {
                ESP_LOGE(TAG_READ_SERIAL, "TIMEOUT UART");
                connect_check=false;
                time_now=0;
                // wait_connect_serial();
                // break;
            }
        }

        if (!connect_check)
        {
            ESP_LOGE(TAG_READ_SERIAL, "connect_check UART");
            dump_uart((uint8_t *) &mess,  sizeof(mess));
        }
        delay(1000);

    }
}

void uart_event_task(void)
{
    // wait_connect_serial();
    xTaskCreate(uart_event, "uart_event", 4096, NULL, 12, NULL);
    // check_timeout();
    // xTaskCreate(check_timeout, "check_timeout", 4096, NULL, 12, NULL);
}

// connect_request mess;
// strncpy(mess.message, request_connect, sizeof(request_connect));

void accept_connect(uint8_t *message)
{
    if (!strncmp(REQUEST_CONNECTION_MSG ,(char *)message,sizeof(REQUEST_CONNECTION_MSG)))
    {
        dump_uart((uint8_t *)RESPONSE_AGREE,sizeof(RESPONSE_AGREE));
        // connect_check=true;
        ESP_LOGI(TAG_READ_SERIAL, RESPONSE_AGREE);
    }
    if (!strncmp(RESPONSE_CONNECTED ,(char *)message,sizeof(RESPONSE_CONNECTED)))
    {
        // dump_uart((uint8_t *)RESPONSE_AGREE,sizeof(RESPONSE_AGREE));
        connect_check=true;
        ESP_LOGI(TAG_READ_SERIAL, "connected");
    }
}

uint8_t wait_connect_serial()
{
    uint8_t reponse_connect_uart[20]; 
    messages_request mess;
    memcpy(mess.message, REQUEST_UART, sizeof(REQUEST_UART));
    esp_wifi_get_mac(ESP_IF_WIFI_STA, mess.mac);
    uint8_t mac[6];
    memcpy(mac, mess.mac, sizeof(mess.mac));
    ESP_LOGI("MAC Address", "MAC: %02X:%02X:%02X:%02X:%02X:%02X",
                mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    while (true)
    {   
        vTaskDelay(pdMS_TO_TICKS(2000));
        ESP_LOGW(TAG_READ_SERIAL,"wait_connect_serial");
        uart_flush(UART_NUM_P2);
        dump_uart((uint8_t *) &mess.message,  sizeof(mess.message));

        memset(reponse_connect_uart, 0, sizeof(reponse_connect_uart));

        uart_read_bytes(UART_NUM_P2, reponse_connect_uart, sizeof(reponse_connect_uart), pdMS_TO_TICKS(200));
            
        if (strcmp((char *)reponse_connect_uart, RESPONSE_AGREE) == 0) 
        {
            ESP_LOGI(TAG_READ_SERIAL, "CONNECTED");
            dump_uart((uint8_t *) RESPONSE_CONNECTED,  sizeof(RESPONSE_CONNECTED));
            break;
        }

    }
    return 1;
}