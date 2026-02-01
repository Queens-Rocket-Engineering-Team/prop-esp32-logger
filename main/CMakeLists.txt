idf_component_register(
    SRCS "main.c"
    INCLUDE_DIRS .
    REQUIRES esp_common esp_driver_i2c control ADS112C04
)
