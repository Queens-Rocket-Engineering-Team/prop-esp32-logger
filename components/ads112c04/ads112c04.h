#pragma once

#include <driver/i2c_master.h>
#include <esp_err.h>
#include <freertos/FreeRTOS.h>
#include <stdint.h>

typedef enum {
    ADS112C04_PIN0 = 0,
    ADS112C04_PIN1 = 1,
    ADS112C04_PIN2 = 2,
    ADS112C04_PIN3 = 3,
    ADS112C04_AVSS = 4
} ads112c04_pin_t;

typedef enum {
    CM_SINGLE_SHOT,
    CM_CONTINUOUS
} conversion_mode_t;

typedef struct {
    StaticSemaphore_t xSemaphoreBufferDRDY;
    SemaphoreHandle_t xSemaphoreDRDY;
    i2c_master_dev_handle_t dev_handle;
    float ref_voltage;
    conversion_mode_t conversion_mode;
    uint8_t addr;
    uint8_t gain;
    bool pga_enabled;
} ads112c04_t;

typedef struct {
    uint8_t addr;
    uint8_t drdy_pin;
    i2c_master_bus_handle_t bus_handle;
    uint32_t bus_frequency;
} ads112c04_config_t;

esp_err_t ads112c04_init(ads112c04_t *ads112c04, const ads112c04_config_t *ads112c04_cfg);

uint8_t ads112c04_get_address(const ads112c04_t *ads112c04);
bool ads112c04_is_gain_valid(uint8_t gain);
bool ads112c04_is_mux_valid(ads112c04_pin_t p_pin, ads112c04_pin_t n_pin);

esp_err_t ads112c04_set_inputs(
    ads112c04_t *ads112c04,
    ads112c04_pin_t p_pin,
    ads112c04_pin_t n_pin,
    uint8_t gain,
    bool pga_enabled
);

esp_err_t ads112c04_set_single_shot(ads112c04_t *ads112c04);

esp_err_t ads112c04_get_single_voltage_reading(
    ads112c04_t *ads112c04,
    float *voltage,
    ads112c04_pin_t p_pin,
    ads112c04_pin_t n_pin,
    uint8_t gain,
    bool pga_enabled
);

esp_err_t ads112c04_get_single_temperature_reading(ads112c04_t *ads112c04, float *temperature);
