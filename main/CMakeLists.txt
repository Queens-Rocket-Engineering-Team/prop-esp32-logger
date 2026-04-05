idf_component_register(
    SRCS "main.c" "setup.c" "wifi/network_manager.c" "wifi/tcp_tools.c" "wifi/ssdp_tools.c"
    INCLUDE_DIRS . ./wifi
    REQUIRES
    control
    ads112c04
    qret_protocol
    esp_config
    esp_common
    esp_driver_i2c
    freertos
    esp_wifi
    esp_timer
    nvs_flash
    esp_event
)
