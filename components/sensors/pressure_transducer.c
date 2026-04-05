#include <esp_check.h>
#include <esp_err.h>
#include <stdbool.h>
#include <stdint.h>

#include "ads112c04.h"
#include "pressure_transducer.h"
#include "sensor.h"

static const char *TAG = "PRESSURE TRANSDUCER";

esp_err_t pressure_transducer_init(pressure_transducer_t *pt, pressure_transducer_config_t *pt_cfg) {
    if (pt == NULL || pt_cfg == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if (pt_cfg->resistor < 0) {
        return ESP_ERR_INVALID_ARG;
    }

    const sensor_config_t sensor_cfg = {
        .adc = pt_cfg->adc,
        .p_pin = pt_cfg->pin,
        .n_pin = ADS112C04_AVSS,
        .gain = 1,
        .pga_enabled = false,
    };

    ESP_RETURN_ON_ERROR(sensor_init(&pt->sensor, &sensor_cfg), TAG, "Failed to initialize pressure transducer sensor");

    pt->resistor = pt_cfg->resistor;
    pt->max_pressure_psi = pt_cfg->max_pressure_psi;
    pt->unit = pt_cfg->unit;
    return ESP_OK;
}

esp_err_t get_pressure_reading(pressure_transducer_t *pt, float *pressure_psi) {
    float voltage = 0;
    ESP_RETURN_ON_ERROR(
        sensor_voltage_reading(&pt->sensor, &voltage), TAG, "Failed to get pressure transducer voltage reading"
    );

    float current_mA = 1000 * voltage / pt->resistor;

    // 4-20 mA pressure transducer
    *pressure_psi = ((current_mA - 4) / 16) * pt->max_pressure_psi;

    return ESP_OK;
}
