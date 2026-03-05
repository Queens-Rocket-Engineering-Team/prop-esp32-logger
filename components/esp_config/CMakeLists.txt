idf_component_register(INCLUDE_DIRS .)

idf_build_get_property(python PYTHON)

# Convert JSON config file to header
set(CONFIG ${CMAKE_SOURCE_DIR}/ESPConfig.json)
set(CONFIG_HEADER ${CMAKE_CURRENT_BINARY_DIR}/esp_config_json.h)
set(CONFIG_CONVERT ${CMAKE_CURRENT_SOURCE_DIR}/esp_config_generate.py)

add_custom_target(generate_config ALL
    COMMAND ${python} ${CONFIG_CONVERT}
            ${CONFIG}
            ${CONFIG_HEADER}
    DEPENDS ${CONFIG} ${CONFIG_CONVERT}
    COMMENT "Generating esp_config_json.h"
)

add_dependencies(${COMPONENT_LIB} generate_config)
target_include_directories(${COMPONENT_LIB} INTERFACE ${CMAKE_CURRENT_BINARY_DIR})