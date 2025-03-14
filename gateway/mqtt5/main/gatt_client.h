#ifndef GATT_CLIENT_H
#define GATT_CLIENT_H

#include "esp_err.h"

/**
 * @brief Initialize the BLE GATT client.
 * 
 * This function starts BLE scanning and attempts to connect to
 * the target BLE peripheral device.
 */
void ble_init(void *pvParameters);

/**
 * @brief Forward data to the BLE GATT server.
 * 
 * This function sends data to the BLE GATT server.
 */
void ble_forward(uint8_t *data, size_t len);
/**
 * @brief BLE event handler for scanning and connection updates.
 * 
 * This function handles GATT client events, including scanning,
 * connection establishment, and data reception.
 */
void ble_event_handler(void* param);

#endif // GATT_CLIENT_H
