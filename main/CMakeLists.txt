idf_component_register(
    SRCS "main.c" "setup.c" "wifi/wifi_tools.c" "wifi/tcp_tools.c" "wifi/ssdp_tools.c"
    INCLUDE_DIRS . ./wifi
    REQUIRES
    esp_common
    esp_driver_i2c
    control
    ADS112C04
    esp_config
    freertos
    esp_wifi
    nvs_flash
    esp_event
)
