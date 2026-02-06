#ifndef ADS112C04_H
#define ADS112C04_H

#include <driver/i2c_master.h>
#include <esp_err.h>
#include <stdint.h>

typedef struct {
    uint8_t _address;
    i2c_master_dev_handle_t _dev_handle;
    uint8_t _pgaGain;
    bool _pga_enabled;
    bool _continuous_mode;
} ADS112C04_t;

esp_err_t ADS112C04_init_i2c(ADS112C04_t *ADS112C04,
                             i2c_master_bus_handle_t *bus_handle);

esp_err_t ADS112C04_set_address(ADS112C04_t *ADS112C04, uint8_t addr);

esp_err_t set_continuous_mode(ADS112C04_t *ADS112C04);
esp_err_t set_single_shot_mode(ADS112C04_t *ADS112C04);

esp_err_t ADS112C04_get_internal_temperature(ADS112C04_t *ADS112C04, float *temperature);

#endif