#pragma once

#include <driver/i2c_master.h>
#include <esp_err.h>
#include <stdint.h>
#include <freertos/FreeRTOS.h>

enum {
    ADS112C04_PIN0 = 0,
    ADS112C04_PIN1 = 1,
    ADS112C04_PIN2 = 2,
    ADS112C04_PIN3 = 3,
    ADS112C04_AVSS = 4
};

typedef enum {
    CM_SINGLE_SHOT,
    CM_CONTINUOUS
} conversion_mode_t;

typedef struct {
    uint8_t address;
    i2c_master_dev_handle_t dev_handle;
    SemaphoreHandle_t xSemaphoreDRDY;
    StaticSemaphore_t xSemaphoreBufferDRDY;
    float ref_voltage;
    uint8_t gain;
    bool pga_enabled;
    conversion_mode_t conversion_mode;
} ads112c04_t;

esp_err_t ads112c04_init_i2c(ads112c04_t *ads112c04, i2c_master_bus_handle_t bus_handle);

esp_err_t ads112c04_set_address(ads112c04_t *ads112c04, uint8_t addr);

esp_err_t ads112c04_get_internal_temperature(ads112c04_t *ads112c04, float *temperature);
