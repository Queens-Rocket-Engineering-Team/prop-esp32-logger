#include "setup.h"
#include "ADS112C04.h"
#include <driver/i2c_master.h>
#include <esp_err.h>

static esp_err_t i2c_master_bus_scan(i2c_master_bus_handle_t bus_handle,
                                     uint8_t address[],
                                     size_t address_len);

static esp_err_t setup_i2c(i2c_master_bus_handle_t *bus_handle,
                           ADS112C04_t devices[],
                           size_t *num_devices);

esp_err_t boot(app_data_t *app_data) {
    if (!app_data) {
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t ret;
    ret = setup_i2c(&app_data->bus_handle, app_data->adcs, &app_data->num_adcs);
    return ret;
}

static esp_err_t setup_i2c(i2c_master_bus_handle_t *bus_handle,
                           ADS112C04_t devices[],
                           size_t *num_devices) {

    if (!bus_handle || !devices || !num_devices) {
        return ESP_ERR_INVALID_ARG;
    }

    // Initialize bus
    i2c_master_bus_config_t i2c_mst_config = {
        .i2c_port = I2C_NUM_0,
        .scl_io_num = SCL_PIN,
        .sda_io_num = SDA_PIN,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = false, // External 1k pullups
    };

    esp_err_t ret;
    ret = i2c_new_master_bus(&i2c_mst_config, bus_handle);
    if (ret != ESP_OK) {
        return ret;
    }

    // Address array terminated with 0xFF, 112 possible addresses
    uint8_t device_addr[113];
    ret = i2c_master_bus_scan(*bus_handle, device_addr, 113);
    if (ret != ESP_OK) {
        return ret;
    }

    // Set up devices on bus
    size_t i;
    for (i = 0; i < MAX_ADCS && device_addr[i] != 0xFF; i++) {

        ret = ADS112C04_set_address(&devices[i], device_addr[i]);
        if (ret != ESP_OK) {
            return ret;
        }

        ret = ADS112C04_init_i2c(&devices[i], bus_handle);
        if (ret != ESP_OK) {
            return ret;
        }
    }
    *num_devices = i;
    return ESP_OK;
}

static esp_err_t i2c_master_bus_scan(i2c_master_bus_handle_t bus_handle,
                                     uint8_t address[],
                                     size_t address_len) {
    if (!address || address_len == 0) {
        return ESP_ERR_INVALID_ARG;
    }

    size_t addr_index = 0;
    esp_err_t ret;
    for (uint8_t addr = 0x08; addr < 0x78; addr++) {
        ret = i2c_master_probe(bus_handle, addr, 100);

        if (ret == ESP_OK) {
            if (addr_index < address_len - 1) { // Leave space for terminator
                address[addr_index++] = addr;
            } else {
                address[addr_index] = 0xFF;
                return ESP_ERR_NO_MEM;
            }
        } else if (ret == ESP_ERR_NOT_FOUND) {
            continue;
        } else {
            address[addr_index] = 0xFF;
            return ret;
        }
    }
    address[addr_index] = 0xFF;
    return ESP_OK;
}