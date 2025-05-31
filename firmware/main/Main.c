// firmware/main/main.c - Ice Pulse ESP32 Main Application
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_sntp.h"
#include "nvs_flash.h"

#include "version.h"
#include "config.h"
#include "wifi_manager.h"
#include "http_client.h"
#include "ota_update.h"
#include "temperature.h"
#include "humidity.h"
#include "data_storage.h"
#include "time_sync.h"

static const char *TAG = "ICE_PULSE_MAIN";

// Task handles
static TaskHandle_t sensor_task_handle = NULL;
static TaskHandle_t data_send_task_handle = NULL;
static TaskHandle_t ota_check_task_handle = NULL;

// Timer handles
static TimerHandle_t sensor_timer = NULL;
static TimerHandle_t heartbeat_timer = NULL;

// Application state
typedef struct {
    bool wifi_connected;
    bool sensors_initialized;
    bool ota_in_progress;
    uint32_t reading_count;
    float last_temperature;
    float last_humidity;
    uint64_t last_reading_time;
} app_state_t;

static app_state_t app_state = {0};

/**
 * @brief Sensor reading task
 */
void sensor_task(void *parameter) {
    float temperature, humidity;
    
    ESP_LOGI(TAG, "Sensor task started");
    
    while (1) {
        // Read temperature sensor
        if (temperature_read(&temperature) == ESP_OK) {
            app_state.last_temperature = temperature;
            ESP_LOGI(TAG, "Temperature: %.2f°C", temperature);
        } else {
            ESP_LOGW(TAG, "Failed to read temperature");
        }
        
        // Read humidity sensor
        if (humidity_read(&humidity) == ESP_OK) {
            app_state.last_humidity = humidity;
            ESP_LOGI(TAG, "Humidity: %.2f%%", humidity);
        } else {
            ESP_LOGW(TAG, "Failed to read humidity");
        }
        
        // Store reading locally
        if (temperature != 0 || humidity != 0) {
            app_state.last_reading_time = esp_timer_get_time() / 1000000; // Convert to seconds
            app_state.reading_count++;
            
            // Store in NVS for offline capability
            data_storage_save_reading(temperature, humidity, app_state.last_reading_time);
        }
        
        // Wait for next reading interval
        vTaskDelay(pdMS_TO_TICKS(SENSOR_READING_INTERVAL_MS));
    }
}

/**
 * @brief Data transmission task
 */
void data_send_task(void *parameter) {
    char json_payload[512];
    
    ESP_LOGI(TAG, "Data send task started");
    
    while (1) {
        if (app_state.wifi_connected && !app_state.ota_in_progress) {
            // Prepare JSON payload
            snprintf(json_payload, sizeof(json_payload),
                "{"
                "\"device_id\":\"%s\","
                "\"firmware_version\":\"%s\","
                "\"timestamp\":%llu,"
                "\"temperature\":%.2f,"
                "\"humidity\":%.2f,"
                "\"reading_count\":%lu,"
                "\"uptime\":%llu"
                "}",
                DEVICE_ID,
                FIRMWARE_VERSION,
                app_state.last_reading_time,
                app_state.last_temperature,
                app_state.last_humidity,
                app_state.reading_count,
                esp_timer_get_time() / 1000000
            );
            
            // Send data to API
            esp_err_t result = http_client_post_data(API_ENDPOINT, json_payload);
            if (result == ESP_OK) {
                ESP_LOGI(TAG, "Data sent successfully");
            } else {
                ESP_LOGW(TAG, "Failed to send data, storing locally");
                // Data is already stored in NVS by sensor task
            }
        }
        
        // Wait for next transmission interval
        vTaskDelay(pdMS_TO_TICKS(DATA_SEND_INTERVAL_MS));
    }
}

/**
 * @brief OTA update check task
 */
void ota_check_task(void *parameter) {
    ESP_LOGI(TAG, "OTA check task started");
    
    while (1) {
        if (app_state.wifi_connected && !app_state.ota_in_progress) {
            ESP_LOGI(TAG, "Checking for OTA updates...");
            
            esp_err_t result = ota_check_for_update(FIRMWARE_VERSION);
            if (result == ESP_OK) {
                ESP_LOGI(TAG, "OTA update available, starting update...");
                app_state.ota_in_progress = true;
                
                // Suspend other tasks during OTA
                if (sensor_task_handle) vTaskSuspend(sensor_task_handle);
                if (data_send_task_handle) vTaskSuspend(data_send_task_handle);
                
                // Perform OTA update
                result = ota_perform_update();
                if (result == ESP_OK) {
                    ESP_LOGI(TAG, "OTA update completed, restarting...");
                    esp_restart();
                } else {
                    ESP_LOGE(TAG, "OTA update failed");
                    app_state.ota_in_progress = false;
                    
                    // Resume tasks
                    if (sensor_task_handle) vTaskResume(sensor_task_handle);
                    if (data_send_task_handle) vTaskResume(data_send_task_handle);
                }
            }
        }
        
        // Check for updates every hour
        vTaskDelay(pdMS_TO_TICKS(OTA_CHECK_INTERVAL_MS));
    }
}

/**
 * @brief WiFi event callback
 */
void wifi_event_callback(wifi_event_t event) {
    switch (event) {
        case WIFI_EVENT_CONNECTED:
            ESP_LOGI(TAG, "WiFi connected");
            app_state.wifi_connected = true;
            
            // Sync time when WiFi connects
            time_sync_init();
            break;
            
        case WIFI_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "WiFi disconnected");
            app_state.wifi_connected = false;
            break;
            
        default:
            break;
    }
}

/**
 * @brief Heartbeat timer callback
 */
void heartbeat_timer_callback(TimerHandle_t timer) {
    // Toggle LED or send heartbeat
    ESP_LOGI(TAG, "Heartbeat - Uptime: %llu s, Readings: %lu", 
             esp_timer_get_time() / 1000000, app_state.reading_count);
}

/**
 * @brief Initialize application
 */
esp_err_t app_init(void) {
    esp_err_t ret = ESP_OK;
    
    ESP_LOGI(TAG, "Starting Ice Pulse Sensor v%s", FIRMWARE_VERSION);
    
    // Initialize NVS
    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    
    // Initialize data storage
    ESP_ERROR_CHECK(data_storage_init());
    
    // Initialize WiFi
    ESP_ERROR_CHECK(wifi_manager_init(wifi_event_callback));
    ESP_ERROR_CHECK(wifi_manager_connect(WIFI_SSID, WIFI_PASSWORD));
    
    // Initialize HTTP client
    ESP_ERROR_CHECK(http_client_init());
    
    // Initialize OTA
    ESP_ERROR_CHECK(ota_update_init(OTA_SERVER_URL));
    
    // Initialize sensors
    ESP_ERROR_CHECK(temperature_init());
    ESP_ERROR_CHECK(humidity_init());
    app_state.sensors_initialized = true;
    
    ESP_LOGI(TAG, "Application initialization completed");
    return ESP_OK;
}

/**
 * @brief Create application tasks
 */
void app_create_tasks(void) {
    // Create sensor reading task
    xTaskCreate(sensor_task, "sensor_task", 4096, NULL, 5, &sensor_task_handle);
    
    // Create data transmission task
    xTaskCreate(data_send_task, "data_send_task", 8192, NULL, 4, &data_send_task_handle);
    
    // Create OTA check task
    xTaskCreate(ota_check_task, "ota_check_task", 8192, NULL, 3, &ota_check_task_handle);
    
    // Create heartbeat timer
    heartbeat_timer = xTimerCreate("heartbeat", pdMS_TO_TICKS(30000), pdTRUE, NULL, heartbeat_timer_callback);
    xTimerStart(heartbeat_timer, 0);
    
    ESP_LOGI(TAG, "Application tasks created");
}

/**
 * @brief Main application entry point
 */
void app_main(void) {
    ESP_LOGI(TAG, "Ice Pulse Sensor starting...");
    ESP_LOGI(TAG, "Firmware Version: %s", FIRMWARE_VERSION);
    ESP_LOGI(TAG, "Device ID: %s", DEVICE_ID);
    ESP_LOGI(TAG, "Free heap: %lu bytes", esp_get_free_heap_size());
    
    // Initialize application
    ESP_ERROR_CHECK(app_init());
    
    // Create and start tasks
    app_create_tasks();
    
    ESP_LOGI(TAG, "Ice Pulse Sensor started successfully");
    
    // Main loop (optional, tasks handle everything)
    while (1) {
        // Monitor system health
        if (esp_get_free_heap_size() < 10000) {
            ESP_LOGW(TAG, "Low memory warning: %lu bytes free", esp_get_free_heap_size());
        }
        
        vTaskDelay(pdMS_TO_TICKS(10000)); // Check every 10 seconds
    }
}