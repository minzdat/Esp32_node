#include "master_espnow_protocol.h"

// Function to initialize allowed slaves with hard-coded values
void test_allowed_connect_slaves_to_nvs(list_slaves_t *test_allowed_connect_slaves) 
{
    // Clear the array
    memset(test_allowed_connect_slaves, 0, sizeof(list_slaves_t) * MAX_SLAVES);
    
    // MASTER hard-coded MAC addresses and statuses
    uint8_t mac1[ESP_NOW_ETH_ALEN] = {0x48, 0x27, 0xe2, 0xc7, 0x21, 0x7c};
    // 48:27:e2:c7:21:7c
    uint8_t mac2[ESP_NOW_ETH_ALEN] = {0xdc, 0xda, 0x0c, 0x0d, 0x41, 0x64};
    uint8_t mac3[ESP_NOW_ETH_ALEN] = {0x34, 0x85, 0x18, 0x03, 0x95, 0x08};
    
    // Add MAC addresses and statuses to the list
    memcpy(test_allowed_connect_slaves[0].peer_addr, mac1, ESP_NOW_ETH_ALEN);
    test_allowed_connect_slaves[0].status = true;   // Offline
    
    memcpy(test_allowed_connect_slaves[1].peer_addr, mac2, ESP_NOW_ETH_ALEN);
    test_allowed_connect_slaves[1].status = false;    // Online
    
    memcpy(test_allowed_connect_slaves[2].peer_addr, mac3, ESP_NOW_ETH_ALEN);
    test_allowed_connect_slaves[2].status = false;    // Offline
}

// Function to print info_slaves
void print_info_slaves(list_slaves_t *info_slaves) 
{
    for (int i = 0; i < MAX_SLAVES; i++) 
    {
        list_slaves_t slave = info_slaves[i];
        
        ESP_LOGI(TAG, "Slave %d:", i);
        ESP_LOGI(TAG, "  MAC Address: %02x:%02x:%02x:%02x:%02x:%02x",
                 slave.peer_addr[0], slave.peer_addr[1], slave.peer_addr[2],
                 slave.peer_addr[3], slave.peer_addr[4], slave.peer_addr[5]);
        ESP_LOGI(TAG, "  Status: %s", slave.status ? "Online" : "Offline");
        ESP_LOGI(TAG, "  Send Errors: %d", slave.send_errors);
        ESP_LOGI(TAG, "  Start Time: %lu", (unsigned long)slave.start_time);
        ESP_LOGI(TAG, "  End Time: %lu", (unsigned long)slave.end_time);
        ESP_LOGI(TAG, "  Number of Retries: %d", slave.number_retry);
        ESP_LOGI(TAG, "  Check Connect Errors: %d", slave.check_connect_errors);
        ESP_LOGI(TAG, "  Count Send: %d", slave.count_send);
        ESP_LOGI(TAG, "  Count Receive: %d", slave.count_receive);
        ESP_LOGI(TAG, "  Count Retry: %d", slave.count_retry);
    }
}

// Function to save info_slaves to NVS
void save_info_slaves_to_nvs(const char *key, list_slaves_t *info_slaves) 
{
    esp_err_t err;
    nvs_handle_t my_handle;

    // Open NVS handle
    err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &my_handle);
    if (err != ESP_OK) 
    {
        ESP_LOGE(TAG, "Failed to open NVS handle, error code: %s", esp_err_to_name(err));
        return;
    }

    // Write the array to NVS
    err = nvs_set_blob(my_handle, key, info_slaves, sizeof(list_slaves_t) * MAX_SLAVES);
    if (err != ESP_OK) 
    {
        ESP_LOGE(TAG, "Failed to write to NVS, error code: %s with key '%s'!", esp_err_to_name(err), key);
    } 
    else 
    {
        ESP_LOGI(TAG, "Successfully saved info_slaves to NVS with key '%s'!", key);
    }

    // Commit written value
    err = nvs_commit(my_handle);
    if (err != ESP_OK) 
    {
        ESP_LOGE(TAG, "Error (%s) committing data to NVS with key '%s'!", esp_err_to_name(err), key);
    }

    // Close NVS handle
    nvs_close(my_handle);
}

// Function to read info_slaves from NVS
void load_info_slaves_from_nvs(const char *key, list_slaves_t *info_slaves) 
{
    esp_err_t err;
    nvs_handle_t my_handle;

    // Open NVS handle
    err = nvs_open(NVS_NAMESPACE, NVS_READONLY, &my_handle);
    if (err != ESP_OK) 
    {
        ESP_LOGE(TAG, "Error (%s) opening NVS handle!", esp_err_to_name(err));
        return;
    }

    // Read the array from NVS
    size_t required_size = sizeof(list_slaves_t) * MAX_SLAVES;
    err = nvs_get_blob(my_handle, key, info_slaves, &required_size);
    if (err == ESP_ERR_NVS_NOT_FOUND) 
    {
        ESP_LOGW(TAG, "No saved info_slaves found in NVS with key '%s'!", key);
        memset(info_slaves, 0, sizeof(list_slaves_t) * MAX_SLAVES);
    } 
    else if (err != ESP_OK) 
    {
        ESP_LOGE(TAG, "Error (%s) reading info_slaves from NVS with key '%s'!", esp_err_to_name(err), key);
    } 
    else 
    {
        ESP_LOGI(TAG, "Loaded info_slaves from NVS successfully with key '%s'!", key);

        // Print the loaded info_slaves
        print_info_slaves(info_slaves);
    }

    // Close NVS handle
    nvs_close(my_handle);
}

// Function to erase NVS by key
void erase_key_in_nvs(const char *key) 
{
    esp_err_t err;
    nvs_handle_t my_handle;

    // Open NVS handle
    err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &my_handle);
    if (err != ESP_OK) 
    {
        ESP_LOGE(TAG, "Failed to open NVS handle, error code: %s", esp_err_to_name(err));
        return;
    }

    // Erase ket
    err = nvs_erase_key(my_handle, key);
    if (err != ESP_OK) 
    {
        ESP_LOGE(TAG, "Failed to erase key in NVS, error code: %s", esp_err_to_name(err));
    } 
    else 
    {
        ESP_LOGI(TAG, "Successfully erased key '%s' in NVS", key);
    }

    // Commit changes
    err = nvs_commit(my_handle);
    if (err != ESP_OK) 
    {
        ESP_LOGE(TAG, "Failed to commit in NVS, error code: %s", esp_err_to_name(err));
    }

    // Close NVS handle
    nvs_close(my_handle);
}

// Function to erase all NVS
void erase_all_in_nvs() 
{
    esp_err_t err;
    nvs_handle_t my_handle;

    // Open NVS handle
    err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &my_handle);
    if (err != ESP_OK) 
    {
        ESP_LOGE(TAG, "Failed to open NVS handle, error code: %s", esp_err_to_name(err));
        return;
    }

    // Erase all nvs
    err = nvs_erase_all(my_handle);
    if (err != ESP_OK) 
    {
        ESP_LOGE(TAG, "Failed to erase all in NVS, error code: %s", esp_err_to_name(err));
    } 
    else 
    {
        ESP_LOGI(TAG, "Successfully erased all in NVS");
    }

    // Commit changes
    err = nvs_commit(my_handle);
    if (err != ESP_OK) 
    {
        ESP_LOGE(TAG, "Failed to commit in NVS, error code: %s", esp_err_to_name(err));
    }

    // Close NVS handle
    nvs_close(my_handle);
}