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
void ble_forward(uint8_t *data, size_t len, uint64_t t_start);
/**
 * @brief BLE event handler for scanning and connection updates.
 * 
 * This function handles GATT client events, including scanning,
 * connection establishment, and data reception.
 */
void ble_event_handler(void* param);

typedef struct {
    uint16_t seq_num;
    uint64_t start_time;
    uint64_t end_time;
} data_entry_t;

#define MAX_BLE_ENTRIES 100
extern data_entry_t *upstream_timings;
extern data_entry_t *downstream_timings;


int extract_sequence_number(const uint8_t *data, size_t len);


#endif // GATT_CLIENT_H
