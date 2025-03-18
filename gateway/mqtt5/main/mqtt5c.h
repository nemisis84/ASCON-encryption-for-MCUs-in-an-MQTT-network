#ifndef MQTT_CLIENT_H
#define MQTT_CLIENT_H

#include "esp_err.h"
#include "mqtt_client.h"

/**
 * @brief Initialize and start the MQTT client.
 * 
 * This function establishes an MQTT connection with the broker,
 * handles subscriptions, and allows publishing messages.
 */
void mqtt5_app_start(void *pvParameters);

/**
 * @brief Publishes data to an MQTT topic.
 * 
 * @param topic The MQTT topic to publish to.
 * @param data The message payload.
 */
void mqtt_publish(const char *topic, const char *data, size_t len);

/**
 * @brief Callback function to handle received MQTT messages.
 * 
 * This function processes messages received from the broker.
 */
void mqtt5_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data);

#endif // MQTT_CLIENT_H
