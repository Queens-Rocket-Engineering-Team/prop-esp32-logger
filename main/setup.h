#ifndef SETUP_H
#define SETUP_H

#include "ads112c04.h"
#include "wifi_tools.h"
#include "config_json.h"
#include <driver/i2c_master.h>
#include <esp_netif.h>
#include <freertos/FreeRTOS.h>

#define SDA_PIN 9
#define SCL_PIN 10

typedef struct {
    ads112c04_t adcs[CONFIG_NUM_ADCS];
    i2c_master_bus_handle_t bus_handle;
} app_ctx_t;

void app_setup(app_ctx_t *app_ctx);

void network_setup(network_ctx_t *network_ctx);

#endif