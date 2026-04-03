#ifndef SETUP_H
#define SETUP_H

#include "ADS112C04.h"
#include "wifi_tools.h"
#include <driver/i2c_master.h>
#include <esp_netif.h>
#include <freertos/FreeRTOS.h>

#define SDA_PIN 9
#define SCL_PIN 10
#define MAX_ADCS 16

typedef struct {
    ADS112C04_t adcs[MAX_ADCS]; // 16 configurable i2c addresses
    size_t num_adcs;
    i2c_master_bus_handle_t bus_handle;
} app_ctx_t;

void app_setup(app_ctx_t *app_ctx);

void network_setup(network_ctx_t *network_ctx);

#endif