#include "wifi_enterprise.h"
#include "gatt_client.h"
#include "mqtt_client.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

static const char *TAG = "MAIN_APP";

void app_main(void) {
    ESP_LOGI(TAG, "Initializing system...");

    // Initialize Non-Volatile Storage (NVS)
    ESP_ERROR_CHECK(nvs_flash_init());

    // Initialize Wi-Fi
    ESP_LOGI(TAG, "Initializing Wi-Fi...");
    wifi_init();

    // Wait until Wi-Fi is connected before proceeding
    vTaskDelay(5000 / portTICK_PERIOD_MS);

    // Initialize BLE
    ESP_LOGI(TAG, "Initializing BLE...");
    ble_init();

    // Initialize MQTT
    ESP_LOGI(TAG, "Initializing MQTT...");
    mqtt_init();

    ESP_LOGI(TAG, "System initialized successfully!");
}
