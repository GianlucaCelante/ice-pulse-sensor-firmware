//firmware/main/config.h - Ice Pulse Configuration
#ifndef CONFIG_H
#define CONFIG_H

// Device Configuration
#define DEVICE_ID "ice-pulse-001"
#define DEVICE_TYPE "temperature_humidity_sensor"

// WiFi Configuration
#define WIFI_SSID "IcePulse-Network"
#define WIFI_PASSWORD "your-wifi-password"
#define WIFI_MAXIMUM_RETRY 5

// API Configuration
#define API_ENDPOINT "http://localhost:8080/api/v1/sensors/data"
#define API_TIMEOUT_MS 10000
#define API_MAX_RETRIES 3

// OTA Configuration - Separato dai dati sensore
#define OTA_SERVER_URL "http://localhost:8092"
#define OTA_CHECK_INTERVAL_HOURS 6                    // Check ogni 6 ore
#define OTA_CHECK_INTERVAL_MS (OTA_CHECK_INTERVAL_HOURS * 60 * 60 * 1000)
#define OTA_TIMEOUT_MS 30000
#define OTA_MAX_RETRIES 3
#define OTA_UPDATE_WINDOW_START_HOUR 2                // Finestra aggiornamenti: 02:00-04:00
#define OTA_UPDATE_WINDOW_END_HOUR 4

// Sensor Configuration - Indipendente da OTA
#define SENSOR_READING_INTERVAL_MINUTES 5             // Lettura ogni 5 minuti
#define SENSOR_READING_INTERVAL_MS (SENSOR_READING_INTERVAL_MINUTES * 60 * 1000)
#define TEMPERATURE_PIN GPIO_NUM_4
#define HUMIDITY_PIN GPIO_NUM_5
#define SENSOR_POWER_PIN GPIO_NUM_2
#define LED_STATUS_PIN GPIO_NUM_18

// Data Configuration - Separato da OTA
#define DATA_SEND_INTERVAL_MINUTES 10                 // Invio ogni 10 minuti
#define DATA_SEND_INTERVAL_MS (DATA_SEND_INTERVAL_MINUTES * 60 * 1000)
#define MAX_OFFLINE_READINGS 100
#define DATA_RETENTION_DAYS 7

// Task Priorities
#define SENSOR_TASK_PRIORITY 5        // Alta priorità per dati critici
#define DATA_SEND_TASK_PRIORITY 4     // Media-alta per trasmissione
#define OTA_CHECK_TASK_PRIORITY 2     // Bassa priorità per aggiornamenti
#define WATCHDOG_TASK_PRIORITY 1      // Minima per monitoring

// Power Management
#define DEEP_SLEEP_ENABLED false
#define DEEP_SLEEP_INTERVAL_US (30 * 60 * 1000000)
#define LOW_BATTERY_THRESHOLD 3.3

// Temperature/Humidity Limits
#define TEMP_MIN_ALARM -25.0
#define TEMP_MAX_ALARM -10.0
#define TEMP_ACCURACY 0.1
#define HUMIDITY_MIN_ALARM 40.0
#define HUMIDITY_MAX_ALARM 80.0
#define HUMIDITY_ACCURACY 1.0

// System Configuration
#define WATCHDOG_TIMEOUT_SEC 30
#define TASK_STACK_SIZE_DEFAULT 4096
#define TASK_STACK_SIZE_HTTP 8192
#define TASK_STACK_SIZE_OTA 10240     // Stack maggiore per OTA

// Debug Configuration
#define DEBUG_LEVEL ESP_LOG_INFO
#define ENABLE_SERIAL_OUTPUT true
#define ENABLE_TELNET_DEBUG false

// Hardware specific
#ifdef CONFIG_IDF_TARGET_ESP32
    #define CHIP_TYPE "ESP32"
#elif CONFIG_IDF_TARGET_ESP32S2
    #define CHIP_TYPE "ESP32-S2"
#elif CONFIG_IDF_TARGET_ESP32C3
    #define CHIP_TYPE "ESP32-C3"
#else
    #define CHIP_TYPE "Unknown"
#endif

#endif