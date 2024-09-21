#include "master_controller.h"

uint8_t mac_address[ESP_NOW_ETH_ALEN] = {0xec, 0xda, 0x3b, 0x54, 0xb5, 0x9c};  // MAC demo
const list_slaves_t default_slave = {0};

void disconnect_node_task(void *pvParameters) 
{
    bool has_online_devices = true;

    while (has_online_devices) 
    {
        ESP_LOGE(TAG, "Task disconnect_node_task");

        has_online_devices = false;  // Initial assumption is that there are no more devices online

        // Browse the list of devices
        for (int i = 0; i < MAX_SLAVES; i++) 
        {
            if (allowed_connect_slaves[i].status)
            {
                has_online_devices = true; // If there is a device online, keep the task running
                memcpy(allowed_connect_slaves[i].message_retry_fail, DISCONNECT_NODE_MSG, sizeof(DISCONNECT_NODE_MSG));
                response_specified_mac(allowed_connect_slaves[i].peer_addr, DISCONNECT_NODE_MSG, false);        
            }
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    for (int i = 0; i < MAX_SLAVES; i++) 
    {
        if (memcmp(&allowed_connect_slaves[i], &default_slave, sizeof(list_slaves_t)) != 0)
        {
            ESP_LOGE(TAG, "Erase table status false");

            memset(&allowed_connect_slaves[i], 0, sizeof(list_slaves_t));
            erase_table_devices(i);
        }
    }

    erase_key_in_nvs("KEY_SLA_ALLOW");

    /* ----------Reconnect to the slaves in the WAITING_CONNECT_SLAVES_LIST---------- */
    vTaskDelay(pdMS_TO_TICKS(15000));
    
    save_info_slaves_to_nvs("KEY_SLA_ALLOW", waiting_connect_slaves);
    load_info_slaves_from_nvs("KEY_SLA_ALLOW", allowed_connect_slaves);
    /* End----------Reconnect to the slaves in the WAITING_CONNECT_SLAVES_LIST---------- */

    // When there are no more devices online, delete the task
    ESP_LOGI(TAG_MASTER_CONTROLLER, "No more online devices. Deleting task.");
    vTaskDelete(NULL);
}

void handle_device(device_type_t device_type, bool state, const uint8_t *dest_mac)
{
    switch (device_type) 
    {
        case DEVICE_RELAY:
            // Controller Relay

            ESP_LOGI(TAG_MASTER_CONTROLLER, "Processing RELAY");
            
            response_specified_mac(dest_mac, CONTROL_RELAY_MSG, false);

            break;
        
        case DISCONNECT_NODE:
            // Disconnect node

            ESP_LOGI(TAG_MASTER_CONTROLLER, "Processing DISCONNECT");

            xTaskCreate(disconnect_node_task, "disconnect_node_task", 4096, NULL, 3, NULL);

            break;

        default:
            // Unspecified case

            break;
    }
}