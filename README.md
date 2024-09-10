# ESP-NOW and MQTT Gateway Project

## Introduction
This repository contains folders related to testing and building wireless data transmission protocols for ESP32 using ESP-NOW, as well as communication through MQTT on ESP32-S3 and ESP32-C3. Below is a description of each folder in the project:

### Main Folders

- **esp-now-master**:
  - Contains the source code for the ESP32 node functioning as the master in testing wireless data transmission via ESP-NOW.
  - The master sends data and tests the transmission range to other nodes.

- **esp-now-slave**:
  - Contains the source code for the ESP32 node functioning as the slave in the testing process.
  - The slave receives data from the master and reports back on the connection status and the data received.

- **master_espnow_protocol**:
  - This folder contains the source code for building the ESP-NOW protocol for the master node.
  - The protocol facilitates wireless data transmission between ESP32-C3 nodes.

- **slave_espnow_protocol**:
  - Contains the source code for the slave node in the ESP-NOW protocol system.
  - The slave node interacts with the master node, receiving data and executing tasks according to the protocol.

- **mqttS3**:
  - This is the source code for the ESP32-S3 gateway to perform UART communication with ESP32-C3 nodes.
  - The S3 gateway connects to the network and sends data from the C3 nodes to the server via MQTT.

## How to Use
1. **esp-now-master & esp-now-slave**:
   - Used to test the range and quality of data transmission between ESP32 nodes via ESP-NOW.
   - The source code should be flashed to two ESP32 devices as master and slave respectively.

2. **master_espnow_protocol & slave_espnow_protocol**:
   - Used to build a complete wireless communication system between ESP32-C3 devices using ESP-NOW.

3. **mqttS3**:
   - Flash this to the ESP32-S3 gateway to serve as an intermediary, transferring data between ESP32-C3 nodes and the MQTT server.