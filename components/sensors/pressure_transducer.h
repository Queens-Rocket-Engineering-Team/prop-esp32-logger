#pragma once

#include <esp_err.h>
#include <stdbool.h>
#include <stdint.h>

#include "qret_protocol.h"
#include "sensor.h"

typedef struct {
    sensor_t sensor;
    float resistor;
    float max_pressure_psi;
    protocol_unit_t unit;
} pressure_transducer_t;

typedef struct {
    ads112c04_t *adc;
    ads112c04_pin_t pin;
    float resistor;
    float max_pressure_psi;
    protocol_unit_t unit;
} pressure_transducer_config_t;

esp_err_t pressure_transducer_init(pressure_transducer_t *pt, pressure_transducer_config_t *pt_cfg);

esp_err_t get_pressure_reading(pressure_transducer_t *pt, float *pressure_psi);
