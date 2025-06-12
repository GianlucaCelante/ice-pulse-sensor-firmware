// firmware/main/main.c - Ice Pulse ESP32 Main Application (OTA Separato)
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
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

static const char *TAG = "ICE_PULSE_MAIN";

// Task handles
static TaskHandle_t sensor_task_handle = NULL;
static TaskHandle_t data_send_task_handle = NULL;
static TaskHandle_t ota_check_task_handle = NULL;
static TaskHandle_t watchdog_task_handle = NULL;

// Application state
typedef struct {
    bool wifi_connected;
    bool sensors_initialized;
    bool ota_in_progress;
    bool system_healthy;
    uint32_t reading_count;
    uint32_t data_send_count;
    uint32_t ota_check_count;
    float last_temperature;
    float last_humidity;
    uint64_t last_reading_time;
    uint64_t last_ota_check_time;
} app_state_t;

static app_state_t app_state = {0};

/**
 * @brief Task di lettura sensori - PRIORITÀ ALTA
 * Esegue letture a intervalli regolari indipendentemente da OTA
 */
void sensor_task(void *parameter) {
    float temperature, humidity;
    
    ESP_LOGI(TAG, "🌡️ Sensor task started (priority: %d)", SENSOR_TASK_PRIORITY);
    
    while (1) {
        if (app_state.sensors_initialized) {
            // Simula lettura sensori (sostituire con codice reale)
            temperature = -18.5f + (esp_random() % 100) / 100.0f;  // Simula variazione
            humidity = 65.0f + (esp_random() % 200 - 100) / 100.0f;
            
            app_state.last_temperature = temperature;
            app_state.last_humidity = humidity;
            app_state.last_reading_time = esp_timer_get_time() / 1000000;
            app_state.reading_count++;
            
            ESP_LOGI(TAG, "📊 Reading #%lu - T:%.2f°C H:%.2f%% [%s]", 
                     app_state.reading_count, temperature, humidity,
                     app_state.ota_in_progress ? "OTA-MODE" : "NORMAL");
            
            // Salva lettura in NVS per resilienza
            // nvs_save_reading(temperature, humidity, app_state.last_reading_time);
        }
        
        // Attendi prossima lettura (indipendente da tutto il resto)
        vTaskDelay(pdMS_TO_TICKS(SENSOR_READING_INTERVAL_MS));
    }
}

/**
 * @brief Task di invio dati - PRIORITÀ MEDIA-ALTA
 * Invia dati al server API senza controlli OTA
 */
void data_send_task(void *parameter) {
    char json_payload[512];
    
    ESP_LOGI(TAG, "📡 Data send task started (priority: %d)", DATA_SEND_TASK_PRIORITY);
    
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
                "\"uptime\":%llu,"
                "\"ota_in_progress\":%s"
                "}",
                DEVICE_ID,
                FIRMWARE_VERSION,
                app_state.last_reading_time,
                app_state.last_temperature,
                app_state.last_humidity,
                app_state.reading_count,
                esp_timer_get_time() / 1000000,
                app_state.ota_in_progress ? "true" : "false"
            );
            
            // Invia dati (NON controlla OTA qui)
            ESP_LOGI(TAG, "📤 Sending data #%lu to API...", app_state.data_send_count + 1);
            
            // Simula invio HTTP (sostituire con http_client_post_data)
            esp_err_t result = ESP_OK; // http_client_post_data(API_ENDPOINT, json_payload);
            
            if (result == ESP_OK) {
                app_state.data_send_count++;
                ESP_LOGI(TAG, "✅ Data sent successfully (#%lu)", app_state.data_send_count);
            } else {
                ESP_LOGW(TAG, "❌ Failed to send data, will retry next cycle");
            }
        } else {
            ESP_LOGD(TAG, "⏳ Waiting for WiFi connection or sensor data...");
        }
        
        // Attendi prossimo invio (completamente separato da OTA)
        vTaskDelay(pdMS_TO_TICKS(DATA_SEND_INTERVAL_MS));
    }
}

/**
 * @brief Task controllo OTA - PRIORITÀ BASSA
 * Controlla aggiornamenti firmware a intervalli programmati
 */
void ota_check_task(void *parameter) {
    struct tm timeinfo;
    time_t now;
    
    ESP_LOGI(TAG, "🔄 OTA check task started (priority: %d, interval: %d hours)", 
             OTA_CHECK_TASK_PRIORITY, OTA_CHECK_INTERVAL_HOURS);
    
    // Primo check dopo 30 secondi (startup)
    vTaskDelay(pdMS_TO_TICKS(30000));
    
    while (1) {
        if (app_state.wifi_connected && !app_state.ota_in_progress) {
            // Ottieni ora corrente
            time(&now);
            localtime_r(&now, &timeinfo);
            
            ESP_LOGI(TAG, "🔍 OTA Check #%lu (Time: %02d:%02d:%02d)", 
                     app_state.ota_check_count + 1, 
                     timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
            
            // Controlla se siamo nella finestra di aggiornamento (opzionale)
            bool in_update_window = (timeinfo.tm_hour >= OTA_UPDATE_WINDOW_START_HOUR && 
                                   timeinfo.tm_hour < OTA_UPDATE_WINDOW_END_HOUR);
            
            // Controlla aggiornamenti disponibili
            esp_err_t check_result = check_ota_server();
            app_state.ota_check_count++;
            app_state.last_ota_check_time = esp_timer_get_time() / 1000000;
            
            if (check_result == ESP_OK) {
                if (in_update_window) {
                    ESP_LOGI(TAG, "🚀 OTA update available and in update window - starting update...");
                    perform_ota_update();
                } else {
                    ESP_LOGI(TAG, "⏰ OTA update available but outside update window (%02d:00-%02d:00)", 
                             OTA_UPDATE_WINDOW_START_HOUR, OTA_UPDATE_WINDOW_END_HOUR);
                }
            } else {
                ESP_LOGD(TAG, "✅ Firmware is up to date");
            }
        } else {
            ESP_LOGD(TAG, "⏳ OTA check skipped (WiFi: %s, OTA in progress: %s)", 
                     app_state.wifi_connected ? "OK" : "NO", 
                     app_state.ota_in_progress ? "YES" : "NO");
        }
        
        // Attendi prossimo controllo OTA
        vTaskDelay(pdMS_TO_TICKS(OTA_CHECK_INTERVAL_MS));
    }
}

/**
 * @brief Controlla server OTA per aggiornamenti
 */
esp_err_t check_ota_server(void) {
    ESP_LOGD(TAG, "🔍 Checking OTA server: %s", OTA_SERVER_URL);
    
    // Simula controllo OTA (sostituire con vera implementazione)
    // return ota_check_for_update(FIRMWARE_VERSION, OTA_SERVER_URL);
    
    // Per ora simula "nessun aggiornamento"
    return ESP_ERR_NOT_FOUND;
}

/**
 * @brief Esegue aggiornamento OTA
 */
esp_err_t perform_ota_update(void) {
    ESP_LOGI(TAG, "🔄 Starting OTA update process...");
    
    app_state.ota_in_progress = true;
    
    // Sospendi task non critici durante OTA (sensori continuano)
    ESP_LOGI(TAG, "⏸️ Suspending non-critical tasks during OTA...");
    if (data_send_task_handle) vTaskSuspend(data_send_task_handle);
    
    // Simula aggiornamento OTA
    ESP_LOGI(TAG, "📥 Downloading firmware update...");
    vTaskDelay(pdMS_TO_TICKS(5000));  // Simula download
    
    ESP_LOGI(TAG, "💾 Installing firmware update...");
    vTaskDelay(pdMS_TO_TICKS(3000));  // Simula installazione
    
    // In caso reale: ota_perform_update() e poi esp_restart()
    ESP_LOGI(TAG, "✅ OTA update completed! Restarting...");
    
    // Per demo, invece di riavviare:
    app_state.ota_in_progress = false;
    if (data_send_task_handle) vTaskResume(data_send_task_handle);
    ESP_LOGI(TAG, "▶️ Resumed normal operations");
    
    return ESP_OK;
}

/**
 * @brief Task watchdog - PRIORITÀ MINIMA
 * Monitora salute del sistema
 */
void watchdog_task(void *parameter) {
    ESP_LOGI(TAG, "🐕 Watchdog task started (priority: %d)", WATCHDOG_TASK_PRIORITY);
    
    while (1) {
        // Controlli di salute sistema
        uint32_t free_heap = esp_get_free_heap_size();
        
        if (free_heap < 10000) {
            ESP_LOGW(TAG, "⚠️ Low memory warning: %lu bytes free", free_heap);
            app_state.system_healthy = false;
        } else {
            app_state.system_healthy = true;
        }
        
        // Log periodico dello stato
        ESP_LOGI(TAG, "💚 System Status - Heap:%luKB | Readings:%lu | DataSent:%lu | OTAChecks:%lu | Healthy:%s",
                 free_heap / 1024,
                 app_state.reading_count,
                 app_state.data_send_count, 
                 app_state.ota_check_count,
                 app_state.system_healthy ? "YES" : "NO");
        
        // Check ogni 60 secondi
        vTaskDelay(pdMS_TO_TICKS(60000));
    }
}

/**
 * @brief WiFi event callback
 */
void wifi_event_callback(int event) {
    switch (event) {
        case 1: // WIFI_EVENT_CONNECTED
            ESP_LOGI(TAG, "📶 WiFi connected");
            app_state.wifi_connected = true;
            break;
            
        case 2: // WIFI_EVENT_DISCONNECTED
            ESP_LOGI(TAG, "📶 WiFi disconnected");
            app_state.wifi_connected = false;
            break;
            
        default:
            break;
    }
}

/**
 * @brief Inizializzazione applicazione
 */
esp_err_t app_init(void) {
    ESP_LOGI(TAG, "🚀 Starting Ice Pulse Sensor v%s", FIRMWARE_VERSION);
    ESP_LOGI(TAG, "📋 Build: %s | Commit: %s", BUILD_TIMESTAMP, GIT_COMMIT);
    
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
    
    // Simula inizializzazione WiFi, sensori, etc.
    app_state.wifi_connected = true;  // Per demo
    app_state.sensors_initialized = true;
    app_state.system_healthy = true;
    
    ESP_LOGI(TAG, "✅ Application initialization completed");
    return ESP_OK;
}

/**
 * @brief Crea task dell'applicazione con priorità separate
 */
void app_create_tasks(void) {
    ESP_LOGI(TAG, "🏗️ Creating application tasks...");
    
    // Task sensori - PRIORITÀ ALTA (sempre attivo)
    xTaskCreate(sensor_task, "sensor_task", TASK_STACK_SIZE_DEFAULT, NULL, 
                SENSOR_TASK_PRIORITY, &sensor_task_handle);
    
    // Task invio dati - PRIORITÀ MEDIA-ALTA
    xTaskCreate(data_send_task, "data_send_task", TASK_STACK_SIZE_HTTP, NULL, 
                DATA_SEND_TASK_PRIORITY, &data_send_task_handle);
    
    // Task controllo OTA - PRIORITÀ BASSA (non interferisce)
    xTaskCreate(ota_check_task, "ota_check_task", TASK_STACK_SIZE_OTA, NULL, 
                OTA_CHECK_TASK_PRIORITY, &ota_check_task_handle);
    
    // Task watchdog - PRIORITÀ MINIMA
    xTaskCreate(watchdog_task, "watchdog_task", TASK_STACK_SIZE_DEFAULT, NULL, 
                WATCHDOG_TASK_PRIORITY, &watchdog_task_handle);
    
    ESP_LOGI(TAG, "✅ All tasks created successfully");
}

/**
 * @brief Main application entry point
 */
void app_main(void) {
    ESP_LOGI(TAG, "🔌 Ice Pulse Sensor starting...");
    ESP_LOGI(TAG, "⚙️ Configuration:");
    ESP_LOGI(TAG, "   📊 Sensor readings every %d minutes", SENSOR_READING_INTERVAL_MINUTES);
    ESP_LOGI(TAG, "   📡 Data transmission every %d minutes", DATA_SEND_INTERVAL_MINUTES);
    ESP_LOGI(TAG, "   🔄 OTA checks every %d hours", OTA_CHECK_INTERVAL_HOURS);
    ESP_LOGI(TAG, "   🕐 OTA update window: %02d:00-%02d:00", 
             OTA_UPDATE_WINDOW_START_HOUR, OTA_UPDATE_WINDOW_END_HOUR);
    
    // Inizializza applicazione
    ESP_ERROR_CHECK(app_init());
    
    // Crea e avvia task
    app_create_tasks();
    
    ESP_LOGI(TAG, "🎉 Ice Pulse Sensor started successfully - all tasks running independently!");
    
    // Main loop minimo (i task gestiscono tutto)
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(30000)); // Sleep 30 secondi
    }
}