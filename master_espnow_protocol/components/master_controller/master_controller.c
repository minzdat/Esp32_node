#include "master_controller.h"

uint8_t mac_address[ESP_NOW_ETH_ALEN] = {0x48, 0x27, 0xe2, 0xc7, 0x1d, 0x18};  // MAC demo

void handle_device(device_type_t device_type, bool state)
{
    switch (device_type) 
    {
        case DEVICE_RELAY:
            // Controller Relay

            ESP_LOGI(TAG_MASTER_CONTROLLER, "Processing RELAY");
            
            response_specified_mac(mac_address, CONTROL_RELAY_MSG, false);

            break;
        
        case DISCONNECT_NODE:
            // Disconnect node

            ESP_LOGI(TAG_MASTER_CONTROLLER, "Processing DISCONNECT");

            // Response disconnect
            for (int i = 0; i < MAX_SLAVES; i++) 
            {
                if (allowed_connect_slaves[i].status) // Check online status
                { 
                    response_specified_mac(allowed_connect_slaves[i].peer_addr, DISCONNECT_NODE_MSG, false);        
                }
            }
            
            break;

        default:
            // Unspecified case

            break;
    }
}