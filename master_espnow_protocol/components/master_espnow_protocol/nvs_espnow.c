#include "master_espnow_protocol.h"

// Function to initialize allowed slaves with hard-coded values
void test_get_allowed_connect_slaves_from_nvs(list_slaves_t *allowed_connect_slaves) {
    // Clear the array
    memset(allowed_connect_slaves, 0, sizeof(list_slaves_t) * MAX_SLAVES);
    
    // MASTER hard-coded MAC addresses and statuses
    uint8_t mac1[ESP_NOW_ETH_ALEN] = {0x1A, 0x2B, 0x3C, 0x4D, 0x5E, 0x6F};
    uint8_t mac2[ESP_NOW_ETH_ALEN] = {0x34, 0x85, 0x18, 0x02, 0xea, 0x44};
    uint8_t mac3[ESP_NOW_ETH_ALEN] = {0x34, 0x85, 0x18, 0x25, 0x2d, 0x94};
    
    // Add MAC addresses and statuses to the list
    memcpy(allowed_connect_slaves[0].peer_addr, mac1, ESP_NOW_ETH_ALEN);
    allowed_connect_slaves[0].status = true;   // Online
    
    memcpy(allowed_connect_slaves[1].peer_addr, mac2, ESP_NOW_ETH_ALEN);
    allowed_connect_slaves[1].status = false;    // Offline
    
    memcpy(allowed_connect_slaves[2].peer_addr, mac3, ESP_NOW_ETH_ALEN);
    allowed_connect_slaves[2].status = false;    // Offline
    
    // Optionally, log the initialized values
    for (int i = 0; i < 3; i++) {
        ESP_LOGI("Init Allowed Slaves", "Slave %d: " MACSTR " Status: %s",
                 i, MAC2STR(allowed_connect_slaves[i].peer_addr), allowed_connect_slaves[i].status ? "Online" : "Offline");
    }
}

// Function to save waiting_connect_slaves to NVS
void save_waiting_connect_slaves_to_nvs(list_slaves_t *waiting_connect_slaves) {
    esp_err_t err;
    nvs_handle_t my_handle;

    // Open NVS handle
    err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &my_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open NVS handle, error code: %s", esp_err_to_name(err));
        return;
    }

    // Write the array to NVS
    err = nvs_set_blob(my_handle, NVS_KEY_SLAVES, waiting_connect_slaves, sizeof(waiting_connect_slaves));
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to write to NVS, error code: %s", esp_err_to_name(err));
    } else {
        ESP_LOGI(TAG, "Successfully saved waiting_connect_slaves to NVS");
    }

    // Commit written value
    err = nvs_commit(my_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to commit to NVS, error code: %s", esp_err_to_name(err));
    }

    // Close NVS handle
    nvs_close(my_handle);
}

// Function to read allowed_connect_slaves from NVS
void load_allowed_connect_slaves_from_nvs(list_slaves_t *allowed_connect_slaves) {
    esp_err_t err;
    nvs_handle_t my_handle;

    // Open NVS handle
    err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &my_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open NVS handle, error code: %s", esp_err_to_name(err));
        return;
    }

    // Read the array from NVS
    size_t required_size = sizeof(allowed_connect_slaves);
    err = nvs_get_blob(my_handle, NVS_KEY_SLAVES, allowed_connect_slaves, &required_size);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGI(TAG, "No data found in NVS, initializing with empty values");
        memset(allowed_connect_slaves, 0, sizeof(list_slaves_t) * MAX_SLAVES);
    } else if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read from NVS, error code: %s", esp_err_to_name(err));
    } else {
        ESP_LOGI(TAG, "Successfully loaded allowed_connect_slaves from NVS");
    }

    // Log the loaded values
    for (int i = 0; i < MAX_SLAVES; i++) {
        ESP_LOGI(TAG, "Waiting allow slave %d: " MACSTR ", status: %s",
                 i, MAC2STR(allowed_connect_slaves[i].peer_addr), allowed_connect_slaves[i].status ? "Online" : "Offline");
    }

    // Close NVS handle
    nvs_close(my_handle);
}

// Function to erase NVS by key
void erase_key_in_nvs(const char *key) {
    esp_err_t err;
    nvs_handle_t my_handle;

    // Open NVS handle
    err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &my_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open NVS handle, error code: %s", esp_err_to_name(err));
        return;
    }

    // Erase ket
    err = nvs_erase_key(my_handle, key);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to erase key in NVS, error code: %s", esp_err_to_name(err));
    } else {
        ESP_LOGI(TAG, "Successfully erased key '%s' in NVS", key);
    }

    // Commit changes
    err = nvs_commit(my_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to commit in NVS, error code: %s", esp_err_to_name(err));
    }

    // Close NVS handle
    nvs_close(my_handle);
}

// Function to erase all NVS
void erase_all_in_nvs() {
    esp_err_t err;
    nvs_handle_t my_handle;

    // Open NVS handle
    err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &my_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open NVS handle, error code: %s", esp_err_to_name(err));
        return;
    }

    // Erase all nvs
    err = nvs_erase_all(my_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to erase all in NVS, error code: %s", esp_err_to_name(err));
    } else {
        ESP_LOGI(TAG, "Successfully erased all in NVS");
    }

    // Commit changes
    err = nvs_commit(my_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to commit in NVS, error code: %s", esp_err_to_name(err));
    }

    // Close NVS handle
    nvs_close(my_handle);
}