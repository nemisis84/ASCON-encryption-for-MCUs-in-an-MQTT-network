/*
 * SPDX-FileCopyrightText: 2022-2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

 #include <stdio.h>
 #include <stdint.h>
 #include <stddef.h>
 #include <string.h>
 #include "esp_system.h"
 #include "nvs_flash.h"
 #include "esp_event.h"
 #include "esp_netif.h"
 #include "protocol_examples_common.h"
 #include "esp_log.h"
 #include "mqtt_client.h"
 #include "mqtt5c.h"
 #include "gatt_client.h"
 
 static const char *TAG = "mqtt5_example";
 
 static esp_mqtt_client_handle_t client = NULL;
 
 static void log_error_if_nonzero(const char *message, int error_code)
 {
     if (error_code != 0) {
         ESP_LOGE(TAG, "Last error %s: 0x%x", message, error_code);
     }
 }
 
 static esp_mqtt5_user_property_item_t user_property_arr[] = {
         {"board", "esp32"},
         {"u", "user"},
         {"p", "password"}
     };
 
 #define USE_PROPERTY_ARR_SIZE   sizeof(user_property_arr)/sizeof(esp_mqtt5_user_property_item_t)
 
 static esp_mqtt5_publish_property_config_t publish_property = {
     .payload_format_indicator = 1,
     .message_expiry_interval = 1000,
     .topic_alias = 0,
     .response_topic = "/topic/test/response",
     .correlation_data = "123456",
     .correlation_data_len = 6,
 };
 
 static esp_mqtt5_subscribe_property_config_t subscribe_property = {
     .subscribe_id = 25555,
     .no_local_flag = false,
     .retain_as_published_flag = false,
     .retain_handle = 0,
     .is_share_subscribe = true,
     .share_name = "group1",
 };
 
 static esp_mqtt5_subscribe_property_config_t subscribe1_property = {
     .subscribe_id = 25555,
     .no_local_flag = true,
     .retain_as_published_flag = false,
     .retain_handle = 0,
 };
 
 static esp_mqtt5_unsubscribe_property_config_t unsubscribe_property = {
     .is_share_subscribe = true,
     .share_name = "group1",
 };
 
 static esp_mqtt5_disconnect_property_config_t disconnect_property = {
     .session_expiry_interval = 60,
     .disconnect_reason = 0,
 };
 
 static void print_user_property(mqtt5_user_property_handle_t user_property)
 {
     if (user_property) {
         uint8_t count = esp_mqtt5_client_get_user_property_count(user_property);
         if (count) {
             esp_mqtt5_user_property_item_t *item = malloc(count * sizeof(esp_mqtt5_user_property_item_t));
             if (esp_mqtt5_client_get_user_property(user_property, item, &count) == ESP_OK) {
                 for (int i = 0; i < count; i ++) {
                     esp_mqtt5_user_property_item_t *t = &item[i];
                     ESP_LOGI(TAG, "key is %s, value is %s", t->key, t->value);
                     free((char *)t->key);
                     free((char *)t->value);
                 }
             }
             free(item);
         }
     }
 }

 void mqtt_ble_forward(const char *topic, int topic_len, const char *data) {
    
    size_t data_len = strlen(data);
    if (topic == NULL || data == NULL || topic_len <= 0 || data_len == 0) {
        ESP_LOGE(TAG, "❌ Invalid input to ble_forward");
        return;
    }

    char clean_topic[128]; 
    if (topic_len >= sizeof(clean_topic)) {
        ESP_LOGE(TAG, "❌ Topic too long!");
        return;
    }

    strncpy(clean_topic, topic, topic_len);
    clean_topic[topic_len] = '\0';  // Ensure null termination

    // Check if it ends with "/PICO"
    if (topic_len < 5 || strcmp(clean_topic + topic_len - 5, "/PICO") != 0) {
        ESP_LOGI(TAG, "🔹 Ignoring message, not for PICO.");
        return;
    }
    
    ESP_LOGI(TAG, "🔹 Forwarding MQTT message to BLE: %s", data);
    ble_forward((uint8_t *)data, data_len);
}


 
 /*
  * @brief Event handler registered to receive MQTT events
  *
  *  This function is called by the MQTT client event loop.
  *
  * @param handler_args user data registered to the event.
  * @param base Event base for the handler(always MQTT Base in this example).
  * @param event_id The id for the received event.
  * @param event_data The data for the event, esp_mqtt_event_handle_t.
  */
 void mqtt5_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
 {
     ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%" PRIi32, base, event_id);
     esp_mqtt_event_handle_t event = event_data;
     int msg_id;
 
     ESP_LOGD(TAG, "free heap size is %" PRIu32 ", minimum %" PRIu32, esp_get_free_heap_size(), esp_get_minimum_free_heap_size());
     switch ((esp_mqtt_event_id_t)event_id) {
     case MQTT_EVENT_CONNECTED:
         ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
 
         esp_mqtt5_client_set_user_property(&subscribe_property.user_property, user_property_arr, USE_PROPERTY_ARR_SIZE);
         esp_mqtt5_client_set_subscribe_property(client, &subscribe_property);
         msg_id = esp_mqtt_client_subscribe(client, "/ascon-e2e/PICO", 1);
         esp_mqtt5_client_delete_user_property(subscribe_property.user_property);
         subscribe_property.user_property = NULL;
         ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);

     case MQTT_EVENT_DISCONNECTED:
         ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
         print_user_property(event->property->user_property);
         break;
     case MQTT_EVENT_SUBSCRIBED:
         ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
         print_user_property(event->property->user_property);
         esp_mqtt5_client_set_publish_property(client, &publish_property);
         msg_id = esp_mqtt_client_publish(client, "/topic/qos0", "data", 0, 0, 0);
         ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);
         break;
     case MQTT_EVENT_UNSUBSCRIBED:
         ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
         print_user_property(event->property->user_property);
         esp_mqtt5_client_set_user_property(&disconnect_property.user_property, user_property_arr, USE_PROPERTY_ARR_SIZE);
         esp_mqtt5_client_set_disconnect_property(client, &disconnect_property);
         esp_mqtt5_client_delete_user_property(disconnect_property.user_property);
         // disconnect_property.user_property = NULL;
         // esp_mqtt_client_disconnect(client);
         break;
     case MQTT_EVENT_PUBLISHED:
         ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
         print_user_property(event->property->user_property);
         break;
    case MQTT_EVENT_DATA:
         ESP_LOGI(TAG, "MQTT_EVENT_DATA");
         print_user_property(event->property->user_property);
         ESP_LOGI(TAG, "TOPIC=%.*s", event->topic_len, event->topic);
         ESP_LOGI(TAG, "DATA=%.*s", event->data_len, event->data);
     
         // ✅ Forward message to BLE
         char mqtt_msg[256];  // Ensure enough space for the message
         snprintf(mqtt_msg, sizeof(mqtt_msg), "%.*s", event->data_len, event->data);
         mqtt_ble_forward(event->topic ,event->topic_len ,mqtt_msg);  // Forward to BLE
         break;
     case MQTT_EVENT_ERROR:
         ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
         print_user_property(event->property->user_property);
         ESP_LOGI(TAG, "MQTT5 return code is %d", event->error_handle->connect_return_code);
         if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
             log_error_if_nonzero("reported from esp-tls", event->error_handle->esp_tls_last_esp_err);
             log_error_if_nonzero("reported from tls stack", event->error_handle->esp_tls_stack_err);
             log_error_if_nonzero("captured as transport's socket errno",  event->error_handle->esp_transport_sock_errno);
             ESP_LOGI(TAG, "Last errno string (%s)", strerror(event->error_handle->esp_transport_sock_errno));
         }
         break;
     default:
         ESP_LOGI(TAG, "Other event id:%d", event->event_id);
         break;
     }
 }
 
 
 void mqtt_publish(const char *topic, const char *data, size_t len) {
     if (client == NULL) {
         ESP_LOGE(TAG, "MQTT client not initialized.");
         return;
     }

     ESP_LOGI(TAG, "🔹 MQTT Debug: Data Before Publish (Raw HEX)");
     ESP_LOG_BUFFER_HEX(TAG, data, len);  

     int msg_id = esp_mqtt_client_publish(client, topic, (const char *)data, len, 1, 0);
     ESP_LOGI(TAG, "Published message to %s: %s (msg_id=%d)", topic, data, msg_id);
 }
 
 
 void mqtt5_app_start(void *pvParameters)
 {
     esp_mqtt5_connection_property_config_t connect_property = {
         .session_expiry_interval = 60,
         .maximum_packet_size = 1024,
         .receive_maximum = 65535,
         .topic_alias_maximum = 2,
         .request_resp_info = true,
         .request_problem_info = true,
         .will_delay_interval = 10,
         .payload_format_indicator = true,
         .message_expiry_interval = 10,
         .response_topic = "/test/response",
         .correlation_data = "123456",
         .correlation_data_len = 6,
     };
 
     esp_mqtt_client_config_t mqtt5_cfg = {
         .broker.address.uri = CONFIG_BROKER_URL,
         .session.protocol_ver = MQTT_PROTOCOL_V_5,
         .network.disable_auto_reconnect = false,
         // .credentials.username = "123",
         // .credentials.authentication.password = "456",
         .session.last_will.topic = "/topic/will",
         .session.last_will.msg = "i will leave",
         .session.last_will.msg_len = 12,
         .session.last_will.qos = 2,
         .session.last_will.retain = true,
         .session.keepalive = 60
     };
 
 
     // Ensure client is assigned once
     if (client == NULL) {
         client = esp_mqtt_client_init(&mqtt5_cfg);
     }
 
     esp_mqtt_client_start(client);
 
     /* Set connection properties and user properties */
     esp_mqtt5_client_set_user_property(&connect_property.user_property, user_property_arr, USE_PROPERTY_ARR_SIZE);
     esp_mqtt5_client_set_user_property(&connect_property.will_user_property, user_property_arr, USE_PROPERTY_ARR_SIZE);
     esp_mqtt5_client_set_connect_property(client, &connect_property);
 
     /* If you call esp_mqtt5_client_set_user_property to set user properties, DO NOT forget to delete them.
      * esp_mqtt5_client_set_connect_property will malloc buffer to store the user_property and you can delete it after
      */
     esp_mqtt5_client_delete_user_property(connect_property.user_property);
     esp_mqtt5_client_delete_user_property(connect_property.will_user_property);
 
     /* The last argument may be used to pass data to the event handler, in this example mqtt_event_handler */
     esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt5_event_handler, NULL);
     esp_mqtt_client_start(client);
 }
 
 // void app_main(void)
 // {
 
 //     ESP_LOGI(TAG, "[APP] Startup..");
 //     ESP_LOGI(TAG, "[APP] Free memory: %" PRIu32 " bytes", esp_get_free_heap_size());
 //     ESP_LOGI(TAG, "[APP] IDF version: %s", esp_get_idf_version());
 
 //     esp_log_level_set("*", ESP_LOG_INFO);
 //     esp_log_level_set("mqtt_client", ESP_LOG_VERBOSE);
 //     esp_log_level_set("mqtt_example", ESP_LOG_VERBOSE);
 //     esp_log_level_set("transport_base", ESP_LOG_VERBOSE);
 //     esp_log_level_set("esp-tls", ESP_LOG_VERBOSE);
 //     esp_log_level_set("transport", ESP_LOG_VERBOSE);
 //     esp_log_level_set("outbox", ESP_LOG_VERBOSE);
 
 //     ESP_ERROR_CHECK(nvs_flash_init());
 //     ESP_ERROR_CHECK(esp_netif_init());
 //     ESP_ERROR_CHECK(esp_event_loop_create_default());
 
 //     /* This helper function configures Wi-Fi or Ethernet, as selected in menuconfig.
 //      * Read "Establishing Wi-Fi or Ethernet Connection" section in
 //      * examples/protocols/README.md for more information about this function.
 //      */
 //     ESP_ERROR_CHECK(example_connect());
 
 //     mqtt5_app_start();
 // }
 