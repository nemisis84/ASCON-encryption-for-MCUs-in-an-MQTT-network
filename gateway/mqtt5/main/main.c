#include "wifi_enterprise.h"
#include "mqtt5c.h"
#include "gatt_client.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_netif.h"

static const char *TAG = "MAIN_APP";

esp_netif_ip_info_t ip_info;

// Function to check if Wi-Fi is connected
bool is_wifi_connected(void) {
    esp_netif_t *netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");

    if (esp_netif_get_ip_info(netif, &ip_info) == ESP_OK) {
        if (ip_info.ip.addr != 0) {
            return true; // Wi-Fi has an IP, meaning it's connected
        }
    }
    return false; // No IP assigned yet
}

void wait_for_wifi_connection() {
    int retries = 0;
    while (!is_wifi_connected()) {
        ESP_LOGI(TAG, "Waiting for Wi-Fi connection...");
        vTaskDelay(pdMS_TO_TICKS(1000));  // Wait 1 second before retrying

        retries++;
        if (retries > 30) { // 30 seconds timeout
            ESP_LOGE(TAG, "Wi-Fi connection failed. Restarting system...");
            esp_restart();
        }
    }
    ESP_LOGI(TAG, "Wi-Fi connected successfully!");
    ESP_LOGI(TAG, "~~~~~~~~~~~");
    ESP_LOGI(TAG, "IP:"IPSTR, IP2STR(&ip_info.ip));
    ESP_LOGI(TAG, "MASK:"IPSTR, IP2STR(&ip_info.netmask));
    ESP_LOGI(TAG, "GW:"IPSTR, IP2STR(&ip_info.gw));
    ESP_LOGI(TAG, "~~~~~~~~~~~");
}


void app_main(void) {
    ESP_LOGI(TAG, "Starting system...");

    // Initialize Wi-Fi (WPA2 Enterprise)
    ESP_LOGI(TAG, "Connecting to Wi-Fi...");
    initialise_wifi();

    // Wait to ensure Wi-Fi connects
    wait_for_wifi_connection();

    // Initialize MQTT Client
    ESP_LOGI(TAG, "Starting MQTT...");
    mqtt5_app_start();

    // Initialize BLE
    ESP_LOGI(TAG, "Starting BLE...");
    ble_init();



    ESP_LOGI(TAG, "System initialized without crashing!");
}
