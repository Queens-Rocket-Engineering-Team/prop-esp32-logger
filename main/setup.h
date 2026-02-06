#ifndef SETUP_H
#define SETUP_H

#include "ADS112C04.h"
#include <driver/i2c_master.h>

#define SDA_PIN 9
#define SCL_PIN 10
#define MAX_ADCS 16

typedef struct {
    ADS112C04_t adcs[MAX_ADCS]; // 16 configurable i2c addresses
    size_t num_adcs;
    i2c_master_bus_handle_t bus_handle;
} app_data_t;

esp_err_t boot(app_data_t *app_data);

#endif