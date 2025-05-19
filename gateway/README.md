# Gateway

This folder contains the source code and configuration for the gateway device in the project. The gateway acts as a bridge between the sensor devices and the MQTT broker, handling BLE communication with the sensors and MQTT communication with the broker. The code is based on libraries from [ESP-IDF](https://github.com/espressif/esp-idf/tree/master/examples), combining mqtt 5 client, wifi enterprice and a BLE client. Th

## Structure

- 

## Features

- BLE GATT client for communication with sensor devices.
- MQTT 5.0 client for communication with the broker.
- Secure communication using certificates.
- Configurable parameters for BLE and MQTT.

## Install Guide

1. Install the required dependencies: