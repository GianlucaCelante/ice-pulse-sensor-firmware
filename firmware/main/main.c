// firmware/main/main.c - Ice Pulse ESP32 Main Application
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "nvs_flash.h"
#include "esp_timer.h"

#include "version.h"
#include "config.h"

static const char *TAG = "ICE_PULSE_MAIN";

// Task handles
static TaskHandle_t sensor_task_handle = NULL;
static TaskHandle_t data_send_task_handle = NULL;
static TaskHandle_t ota_check_task_handle = NULL;

// Application state
typedef struct {
    bool wifi_connected;
    bool sensors_initialized;
    bool ota_in_progress;
    uint32_t reading_count;
    uint32_t data_send_count;
    uint32_t ota_check_count;
    float last_temperature;
    float last_humidity;
    uint64_t last_reading_time;
    uint64_t last_ota_check_time;
} app_state_t;

static app_state_t app_state = {0};

// Dichiarazioni delle funzioni
esp_err_t check_ota_server(void);
esp_err_t perform_ota_update(void);

/**
 * @brief Task di lettura sensori
 */
void sensor_task(void *parameter) {
    ESP_LOGI(TAG, "Sensor task started");
    
    while (1) {
        if (app_state.sensors_initialized) {
            // Simula lettura sensori
            app_state.last_temperature = -18.5f + (esp_random() % 100) / 100.0f;
            app_state.last_humidity = 65.0f + (esp_random() % 200 - 100) / 100.0f;
            app_state.last_reading_time = esp_timer_get_time() / 1000000;
            app_state.reading_count++;
            
            ESP_LOGI(TAG, "Reading #%lu - T:%.2f°C H:%.2f%%", 
                     app_state.reading_count, 
                     app_state.last_temperature, 
                     app_state.last_humidity);
        }
        
        // Lettura ogni 5 minuti
        vTaskDelay(pdMS_TO_TICKS(5 * 60 * 1000));
    }
}

/**
 * @brief Task di invio dati
 */
void data_send_task(void *parameter) {
    char json_payload[512];
    
    ESP_LOGI(TAG, "Data send task started");
    
    while (1) {
        if (app_state.wifi_connected && app_state.reading_count > 0) {
            // Prepara payload JSON
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
                (unsigned long long)app_state.last_reading_time,
                app_state.last_temperature,
                app_state.last_humidity,
                app_state.reading_count,
                (unsigned long long)(esp_timer_get_time() / 1000000)
            );
            
            ESP_LOGI(TAG, "Sending data #%lu to API...", app_state.data_send_count + 1);
            
            // TODO: Implementare invio HTTP reale
            app_state.data_send_count++;
            ESP_LOGI(TAG, "Data sent successfully");
        }
        
        // Invio ogni 10 minuti
        vTaskDelay(pdMS_TO_TICKS(10 * 60 * 1000));
    }
}

/**
 * @brief Task controllo OTA
 */
void ota_check_task(void *parameter) {
    ESP_LOGI(TAG, "OTA check task started");
    
    // Primo check dopo 30 secondi
    vTaskDelay(pdMS_TO_TICKS(30000));
    
    while (1) {
        if (app_state.wifi_connected && !app_state.ota_in_progress) {
            ESP_LOGI(TAG, "Checking for OTA updates...");
            
            esp_err_t check_result = check_ota_server();
            app_state.ota_check_count++;
            
            if (check_result == ESP_OK) {
                ESP_LOGI(TAG, "Update available, starting OTA...");
                perform_ota_update();
            } else {
                ESP_LOGI(TAG, "No updates available");
            }
        }
        
        // Check ogni 6 ore
        vTaskDelay(pdMS_TO_TICKS(6 * 60 * 60 * 1000));
    }
}

/**
 * @brief Controlla server OTA per aggiornamenti
 */
esp_err_t check_ota_server(void) {
    ESP_LOGD(TAG, "Checking OTA server: %s", OTA_SERVER_URL);
    
    // TODO: Implementare controllo OTA reale
    // Per ora simula "nessun aggiornamento"
    return ESP_ERR_NOT_FOUND;
}

/**
 * @brief Esegue aggiornamento OTA
 */
esp_err_t perform_ota_update(void) {
    ESP_LOGI(TAG, "Starting OTA update...");
    
    app_state.ota_in_progress = true;
    
    // TODO: Implementare aggiornamento OTA reale
    // esp_https_ota(&ota_config);
    
    app_state.ota_in_progress = false;
    
    return ESP_OK;
}

/**
 * @brief Inizializzazione applicazione
 */
esp_err_t app_init(void) {
    ESP_LOGI(TAG, "Starting Ice Pulse Sensor v%s", FIRMWARE_VERSION);
    
    // Inizializza NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    
    // Inizializza networking
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    
    // Per demo, simula connessione
    app_state.wifi_connected = true;
    app_state.sensors_initialized = true;
    
    return ESP_OK;
}

/**
 * @brief Main application entry point
 */
void app_main(void) {
    ESP_LOGI(TAG, "Ice Pulse Sensor starting...");
    
    // Inizializza applicazione
    ESP_ERROR_CHECK(app_init());
    
    // Crea task
    xTaskCreate(sensor_task, "sensor_task", 4096, NULL, 5, &sensor_task_handle);
    xTaskCreate(data_send_task, "data_send_task", 8192, NULL, 4, &data_send_task_handle);
    xTaskCreate(ota_check_task, "ota_check_task", 8192, NULL, 2, &ota_check_task_handle);
    
    ESP_LOGI(TAG, "All tasks running!");
    
    // Main loop
    while (1) {
        ESP_LOGI(TAG, "Status - Reads:%lu | Sent:%lu | OTA:%lu", 
                 app_state.reading_count, 
                 app_state.data_send_count, 
                 app_state.ota_check_count);
        vTaskDelay(pdMS_TO_TICKS(60000));
    }
}