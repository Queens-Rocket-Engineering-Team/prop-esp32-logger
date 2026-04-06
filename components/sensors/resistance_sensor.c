#include <esp_check.h>
#include <esp_err.h>
#include <stdbool.h>
#include <stdint.h>

#include "ads112c04.h"
#include "resistance_sensor.h"
#include "sensor.h"

static const char *TAG = "RESISTANCE_SENSOR";

esp_err_t resistance_sensor_init(resistance_sensor_t *resistance_sensor, const resistance_sensor_config_t *resistance_sensor_cfg) {
    if (resistance_sensor == NULL || resistance_sensor_cfg == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    const sensor_config_t sensor_cfg = {
        .adc = resistance_sensor_cfg->adc,
        .p_pin = resistance_sensor_cfg->pin,
        .n_pin = ADS112C04_AVSS,
        .gain = 4,
        .pga_enabled = false,
    };

    ESP_RETURN_ON_ERROR(sensor_init(&resistance_sensor->sensor, &sensor_cfg), TAG, "Failed to initialize resistance sensor");

    resistance_sensor->injected_current_uA = resistance_sensor_cfg->injected_current_uA;
    resistance_sensor->r_short = resistance_sensor_cfg->r_short;
    resistance_sensor->unit = resistance_sensor_cfg->unit;
    return ESP_OK;
}

esp_err_t get_resistance_reading(resistance_sensor_t *resistance_sensor, float *resistance) {
    if (resistance_sensor == NULL || resistance == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    // TODO enable idac here
    
    float voltage = 0;
    ESP_RETURN_ON_ERROR(
        sensor_voltage_reading(&resistance_sensor->sensor, &voltage), TAG, "Failed to get resistance voltage reading"
    );

    // TODO disable idac here

    const float resistance_ohms = (1e6 * voltage / resistance_sensor->injected_current_uA) - resistance_sensor->r_short;
    if (resistance_sensor->unit == RESISTANCE_SENSOR_OHMS) {
        *resistance = resistance_ohms;
    } else {
        return ESP_ERR_INVALID_ARG;
    }
    return ESP_OK;
}
