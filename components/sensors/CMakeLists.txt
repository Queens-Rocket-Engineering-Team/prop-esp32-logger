idf_component_register(
    SRCS "sensor.c" "pressure_transducer.c" "thermocouple.c"
    INCLUDE_DIRS .
    REQUIRES
    ads112c04
    qret_protocol
)