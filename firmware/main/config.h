//firmware/main/config.h - Ice Pulse Configuration
#ifndef CONFIG_H
#define CONFIG_H

// Device Configuration
#define DEVICE_ID "ice-pulse-001"
#define DEVICE_TYPE "temperature_humidity_sensor"

// WiFi Configuration (will be moved to NVS in production)
#define WIFI_SSID "IcePulse-Network"
#define WIFI_PASSWORD "your-wifi-password"
#define WIFI_MAXIMUM_RETRY 5

// API Configuration
#define API_ENDPOINT "http://localhost:8080/api/v1/sensors/data"
#define API_TIMEOUT_MS 10000
#define API_MAX_RETRIES 3

// OTA Configuration
#define OTA_SERVER_URL "http://localhost:8092"
#define OTA_CHECK_INTERVAL_MS (60 * 60 * 1000) // 1 hour
#define OTA_TIMEOUT_MS 30000

// Sensor Configuration
#define SENSOR_READING_INTERVAL_MS (5 * 60 * 1000) // 5 minutes
#define TEMPERATURE_PIN GPIO_NUM_4
#define HUMIDITY_PIN GPIO_NUM_5
#define SENSOR_POWER_PIN GPIO_NUM_2
#define LED_STATUS_PIN GPIO_NUM_18

// Data Configuration
#define DATA_SEND_INTERVAL_MS (10 * 60 * 1000) // 10 minutes
#define MAX_OFFLINE_READINGS 100
#define DATA_RETENTION_DAYS 7

// Power Management
#define DEEP_SLEEP_ENABLED false
#define DEEP_SLEEP_INTERVAL_US (30 * 60 * 1000000) // 30 minutes
#define LOW_BATTERY_THRESHOLD 3.3 // Volts

// Temperature Sensor Limits (for alerts)
#define TEMP_MIN_ALARM -25.0
#define TEMP_MAX_ALARM -10.0
#define TEMP_ACCURACY 0.1

// Humidity Sensor Limits
#define HUMIDITY_MIN_ALARM 40.0
#define HUMIDITY_MAX_ALARM 80.0
#define HUMIDITY_ACCURACY 1.0

// System Configuration
#define WATCHDOG_TIMEOUT_SEC 30
#define TASK_STACK_SIZE_DEFAULT 4096
#define TASK_STACK_SIZE_HTTP 8192

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