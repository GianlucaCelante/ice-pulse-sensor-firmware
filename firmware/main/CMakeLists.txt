idf_component_register(
    SRCS 
        "main.c"
    
    INCLUDE_DIRS 
        "."
        "sensors"
    
    REQUIRES 
        nvs_flash
        esp_wifi
        esp_http_client
        esp_https_ota
        json
        lwip
        esp_netif
        esp_event
        driver
        esp_timer
)