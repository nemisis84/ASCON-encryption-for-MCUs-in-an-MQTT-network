# Gateway

This folder contains the source code and configuration for the gateway device in the project. The gateway acts as a bridge between the sensor devices and the MQTT broker, handling BLE communication with the sensors and MQTT communication with the broker. The code is based on libraries from [ESP-IDF](https://github.com/espressif/esp-idf/tree/master/examples), combining mqtt 5 client, wifi enterprice and a BLE client. It's highly recommended to use VS coed and download the ESP-IDF extension. This makes it easier to build and flash the ESP32-C3. 

## Structure

- main/ --> Includes alle the .c files and certificates. Separate files for MQTT, BLE and Wi-Fi and a main.c putting it all together. 
- generate_certs/ --> Contains script for generating certificates. 
- sdkconfig* --> Contains project configs. 

## Important info

- THe device needs to be reset if the BLE connection is disconnected. No automatic reconnection is implemented. 
- The Wi-Fi password is currently hardcoded into the sdkconfig file. This is an UNSECURE way of storing the password, and it should be flashed to the device another way. Make sure that you don't push your secrets to github (file is currently under .gitignore). 

## Install Guide

Installing the ESP-SDK VS code extension should be sufficent. Make sure to open VS code with /gateway_app as root folder. The task bar at the bottom will have options such as "build", "flash" and monitor. Furthermore, you can select the port that you use to flash to the device. 