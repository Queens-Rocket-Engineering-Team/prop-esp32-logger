idf_component_register(
    SRCS "main.c" "setup.c"
    INCLUDE_DIRS .
    REQUIRES esp_common esp_driver_i2c control ADS112C04 ESPConfig freertos
)
