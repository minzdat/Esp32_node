#ifndef MASTER_CONTROLLER_H
#define MASTER_CONTROLLER_H

#include <stdio.h>
#include <stdbool.h>
#include "esp_log.h"
#include "master_espnow_protocol.h"

#define TAG_MASTER_CONTROLLER           "MASTER_CONTROLLER"
#define CONTROL_RELAY_MSG               "CONTROL_relay"
#define DISCONNECT_NODE_MSG             "DISCONNECT_node"

typedef enum {
    DEVICE_RELAY,
    DISCONNECT_NODE,
    DEVICE_UNKNOWN
} device_type_t;

void handle_device(device_type_t device_type, bool state, const uint8_t *dest_mac); 

#endif //MASTER_CONTROLLER_H