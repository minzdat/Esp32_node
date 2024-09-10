#include "slave_espnow_protocol.h"

void log_data_from_nvs() 
{
    ESP_LOGI(TAG, "____________________________________________________");
    
    // Log the value of the entire s_master_unicast_mac
    ESP_LOGI(TAG, "s_master_unicast_mac.connected = %s", s_master_unicast_mac.connected ? "true" : "false");
    ESP_LOGI(TAG, "s_master_unicast_mac.count_keep_connect = %" PRId32, s_master_unicast_mac.count_keep_connect);

    // Log peer address (if you need to log MAC address)
    ESP_LOGI(TAG, "s_master_unicast_mac.peer_addr = %02x:%02x:%02x:%02x:%02x:%02x",
             s_master_unicast_mac.peer_addr[0], s_master_unicast_mac.peer_addr[1],
             s_master_unicast_mac.peer_addr[2], s_master_unicast_mac.peer_addr[3],
             s_master_unicast_mac.peer_addr[4], s_master_unicast_mac.peer_addr[5]);

    ESP_LOGI(TAG, "____________________________________________________");
}

// Function to remove the value of s_master_unicast_mac.connected from NVS
void erase_nvs(const char *key) 
{
    esp_err_t err;
    nvs_handle_t my_handle;

    // Open the NVS handle
    err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &my_handle);
    if (err != ESP_OK) 
    {
        ESP_LOGE(TAG, "Error (%s) opening NVS handle!", esp_err_to_name(err));
        return;
    }

    // Delete key from NVS
    err = nvs_erase_key(my_handle, key);
    if (err != ESP_OK) 
    {
        ESP_LOGE(TAG, "Error (%s) erasing key '%s' from NVS!", esp_err_to_name(err), key);
    } 
    else 
    {
        ESP_LOGI(TAG, "Erased key '%s' from NVS successfully!", key);
    }

    // Commit changes and close the NVS handle
    err = nvs_commit(my_handle);
    if (err != ESP_OK) 
    {
        ESP_LOGE(TAG, "Error (%s) committing changes to NVS!", esp_err_to_name(err));
    }

    nvs_close(my_handle);
}


// Function to load values ​​from NVS into members of the structure
void load_from_nvs(const char *key_connected, const char *key_count, const char *key_peer_addr, mac_master_t *mac_master) 
{
    esp_err_t err;
    nvs_handle_t my_handle;
    uint8_t value = 0; 

    // Open the NVS handle
    err = nvs_open(NVS_NAMESPACE, NVS_READONLY, &my_handle);
    if (err != ESP_OK) 
    {
        ESP_LOGE(TAG, "Error (%s) opening NVS handle!", esp_err_to_name(err));
        return;
    }

    // Read the value of connected from NVS
    err = nvs_get_u8(my_handle, key_connected, &value);
    if (err == ESP_ERR_NVS_NOT_FOUND) 
    {
        ESP_LOGW(TAG, "No saved connected value found in NVS with key_connected '%s'!", key_connected);
        mac_master->connected = false; 
    } 
    else if (err != ESP_OK) 
    {
        ESP_LOGE(TAG, "Error (%s) reading connected value from NVS with key_connected '%s'!", esp_err_to_name(err), key_connected);
    } 
    else 
    {
        ESP_LOGI(TAG, "Loaded connected value from NVS successfully with key_connected '%s'!", key_connected);
        mac_master->connected = (bool)value;
    }

    // Read the value of count_keep_connect from NVS
    err = nvs_get_i32(my_handle, key_count, &mac_master->count_keep_connect);
    if (err == ESP_ERR_NVS_NOT_FOUND) 
    {
        ESP_LOGW(TAG, "No saved count_keep_connect value found in NVS with key_count '%s'!", key_count);
        mac_master->count_keep_connect = 0;
    } 
    else if (err != ESP_OK) 
    {
        ESP_LOGE(TAG, "Error (%s) reading count_keep_connect value from NVS with key_count '%s'!", esp_err_to_name(err), key_count);
    } 
    else 
    {
        ESP_LOGI(TAG, "Loaded count_keep_connect value from NVS successfully with key_count '%s'!", key_count);
    }


    // Read the value of peer_addr from NVS
    size_t required_size = ESP_NOW_ETH_ALEN; // Size of peer_addr array
    err = nvs_get_blob(my_handle, key_peer_addr, NULL, &required_size);
    if (err == ESP_ERR_NVS_NOT_FOUND) 
    {
        ESP_LOGW(TAG, "No saved peer_addr value found in NVS with key_peer_addr '%s'!", key_peer_addr);
        memset(mac_master->peer_addr, 0, ESP_NOW_ETH_ALEN); // Initialize with zeros if not found
    } 
    else if (err != ESP_OK) 
    {
        ESP_LOGE(TAG, "Error (%s) getting peer_addr blob size from NVS with key_peer_addr '%s'!", esp_err_to_name(err), key_peer_addr);
    } 
    else 
    {
        err = nvs_get_blob(my_handle, key_peer_addr, mac_master->peer_addr, &required_size);
        if (err != ESP_OK) 
        {
            ESP_LOGE(TAG, "Error (%s) reading peer_addr value from NVS with key_peer_addr '%s'!", esp_err_to_name(err), key_peer_addr);
        } 
        else 
        {
            ESP_LOGI(TAG, "Loaded peer_addr value from NVS successfully with key_peer_addr '%s'!", key_peer_addr);
        }
    }

    // Close the NVS handle
    nvs_close(my_handle);
}

// Function to save the value of s_master_unicast_mac.connected to NVS
void save_to_nvs(const char *key_connected, const char *key_count, const char *key_peer_addr, bool connected, int count_keep_connect, const uint8_t *peer_addr) 
{
    esp_err_t err;
    nvs_handle_t my_handle;

    // Open the NVS handle
    err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &my_handle);
    if (err != ESP_OK) 
    {
        ESP_LOGE(TAG, "Error (%s) opening NVS handle!", esp_err_to_name(err));
        return;
    }

    // Save the value of connected to NVS
    err = nvs_set_u8(my_handle, key_connected, connected);
    if (err != ESP_OK) 
    {
        ESP_LOGE(TAG, "Error (%s) saving connected value to NVS with key '%s'!", esp_err_to_name(err), key_connected);
    } 
    else 
    {
        ESP_LOGI(TAG, "Saved connected value to NVS successfully with key '%s'!", key_connected);
    }

    // Save the value of count_keep_connect to NVS
    err = nvs_set_i32(my_handle, key_count, count_keep_connect);
    if (err != ESP_OK) 
    {
        ESP_LOGE(TAG, "Error (%s) saving count_keep_connect value to NVS with key '%s'!", esp_err_to_name(err), key_count);
    } 
    else 
    {
        ESP_LOGI(TAG, "Saved count_keep_connect value to NVS successfully with key '%s'!", key_count);
    }

    // Save the value of peer_addr to NVS
    err = nvs_set_blob(my_handle, key_peer_addr, peer_addr, ESP_NOW_ETH_ALEN);
    if (err != ESP_OK) 
    {
        ESP_LOGE(TAG, "Error (%s) saving peer_addr value to NVS with key '%s'!", esp_err_to_name(err), key_peer_addr);
    } 
    else 
    {
        ESP_LOGI(TAG, "Saved peer_addr value to NVS successfully with key '%s'!", key_peer_addr);
    }

    // Commit the changes and close the NVS handle
    err = nvs_commit(my_handle);
    if (err != ESP_OK) 
    {
        ESP_LOGE(TAG, "Error (%s) committing changes to NVS!", esp_err_to_name(err));
    }

    nvs_close(my_handle);
}