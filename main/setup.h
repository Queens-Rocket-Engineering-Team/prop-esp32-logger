#pragma once

#include <driver/i2c_master.h>
#include <esp_netif.h>
#include <freertos/FreeRTOS.h>

#include "ads112c04.h"
#include "config_json.h"
#include "wifi_tools.h"

typedef struct {
    network_ctx_t *network_ctx;
    ads112c04_t adcs[CONFIG_NUM_ADCS];
    config_sensor_t sensors[CONFIG_NUM_SENSORS];
    uint32_t ts_offset;
    uint8_t sequence;
    portMUX_TYPE sequence_spinlock;
    i2c_master_bus_handle_t bus_handle;
} app_ctx_t;

esp_err_t app_setup(app_ctx_t *app_ctx, network_ctx_t *network_ctx);