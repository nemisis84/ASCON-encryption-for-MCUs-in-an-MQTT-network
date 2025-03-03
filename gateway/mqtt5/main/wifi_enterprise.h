#ifndef WIFI_ENTERPRISE_H
#define WIFI_ENTERPRISE_H

#include "esp_err.h"
#include "esp_event.h"

/**
 * @brief Initialize the Wi-Fi Enterprise (WPA2) connection.
 * 
 * This function initializes Wi-Fi in WPA2 Enterprise mode and connects
 * to the configured SSID using PEAP or TTLS authentication.
 */
void initialise_wifi(void);

/**
 * @brief Event handler for Wi-Fi-related events.
 * 
 * This function is used to monitor Wi-Fi connection status.
 */
void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);

#endif // WIFI_ENTERPRISE_H
