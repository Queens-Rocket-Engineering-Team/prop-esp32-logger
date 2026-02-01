#ifndef ADS112C04_H
#define ADS112C04_H

#include <driver/i2c_master.h>
#include <esp_err.h>
#include <stdbool.h>
#include <stdint.h>

typedef struct {
    uint8_t address;
    i2c_master_dev_handle_t dev_handle;
    uint8_t pgaGain;
} ADS112C04_t;

esp_err_t ADS112C04_init_i2c(ADS112C04_t *ADS112C04,
                             i2c_master_bus_handle_t *bus_handle);

uint8_t ADS112C04_get_address(ADS112C04_t *ADS112C04);

#endif