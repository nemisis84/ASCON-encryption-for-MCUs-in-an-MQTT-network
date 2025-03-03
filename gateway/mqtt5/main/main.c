#include "wifi_enterprise.h"
#include "mqtt5c.h"
#include "gatt_client.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

static const char *TAG = "MAIN_APP";

void app_main(void) {
    ESP_LOGI(TAG, "Starting system...");

    // Initialize Wi-Fi (WPA2 Enterprise)
    ESP_LOGI(TAG, "Connecting to Wi-Fi...");
    initialise_wifi();

    // Wait to ensure Wi-Fi connects
    vTaskDelay(pdMS_TO_TICKS(5000));

    // Initialize BLE
    ESP_LOGI(TAG, "Starting BLE...");
    // ble_init();

    // Initialize MQTT Client
    ESP_LOGI(TAG, "Starting MQTT...");
    mqtt5_app_start();

    ESP_LOGI(TAG, "System initialized successfully!");
}
