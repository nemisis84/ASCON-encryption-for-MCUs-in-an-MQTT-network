#include "wifi_enterprise.h"
#include "mqtt5c.h"
#include "gatt_client.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_netif.h"

static const char *TAG = "MAIN_APP";

data_entry_t *upstream_timings = NULL;
data_entry_t *downstream_timings = NULL;


esp_netif_ip_info_t ip_info;

// Function to check if Wi-Fi is connected
bool is_wifi_connected(void) {
    esp_netif_t *netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");

    if (!netif) {
        ESP_LOGE(TAG, "Failed to get Wi-Fi interface handle!");
        return false;
    }

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

void mqtt_task(void *pvParameters) {
    mqtt5_app_start(pvParameters);
    vTaskDelete(NULL);  // Delete the task when done
}

void ble_task(void *pvParameters) {
    ble_init(pvParameters);
    vTaskDelete(NULL);
}


void init_processing_logging(){
    if (upstream_timings) {
        free(upstream_timings);
        upstream_timings = NULL;
    }
    if (downstream_timings) {
        free(downstream_timings);
        downstream_timings = NULL;
    }

    upstream_timings = calloc(MAX_BLE_ENTRIES, sizeof(data_entry_t));
    downstream_timings = calloc(MAX_BLE_ENTRIES, sizeof(data_entry_t));

    if (!upstream_timings || !downstream_timings) {
        ESP_LOGE(TAG, "Failed to allocate memory for timing structs");
        abort();
    }
}


void app_main(void *pvParameters) {
    ESP_LOGI(TAG, "Starting system...");

    init_processing_logging();

    // Initialize Wi-Fi (WPA2 Enterprise)
    ESP_LOGI(TAG, "Connecting to Wi-Fi...");
    initialise_wifi();

    // Wait to ensure Wi-Fi connects
    wait_for_wifi_connection();

    // Initialize MQTT Client
    ESP_LOGI(TAG, "Starting MQTT...");
    xTaskCreate(mqtt_task, "mqtt_task", 8192, NULL, 5, NULL);

    // Initialize BLE
    xTaskCreate(ble_task, "ble_task", 8192, NULL, 5, NULL);

    // ble_init();

    ESP_LOGI(TAG, "System initialized without crashing!");
}
