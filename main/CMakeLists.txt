idf_component_register(
    SRCS "main.c" "setup.c" "wifi_tools.c" "tcp_tools.c" "ssdp_tools.c"
    INCLUDE_DIRS .
    REQUIRES
    esp_common
    esp_driver_i2c
    control ADS112C04
    ESPConfig freertos
    esp_wifi
    nvs_flash
    esp_event
)
