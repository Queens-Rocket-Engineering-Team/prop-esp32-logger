#include "ADS112C04.h"
#include "control.h"
#include <driver/i2c_master.h>
#include <esp_check.h>
#include <esp_err.h>

#define SDA_PIN 9
#define SCL_PIN 10

typedef struct {
    ADS112C04_t * const adcs;
    uint8_t num_adcs;
    i2c_master_bus_handle_t bus_handle;
} app_data_t;

esp_err_t setup_i2c(i2c_master_bus_handle_t *bus_handle,
                    ADS112C04_t *devices[],
                    size_t numDevices);
esp_err_t boot(app_data_t *app_data);
void run(app_data_t *app_data);

void app_main(void) {

    static ADS112C04_t test_adc[5];
    static app_data_t app_data = {
        .adcs = test_adc,
        .num_adcs = sizeof(test_adc) / sizeof(test_adc[0]),
    };

    boot(&app_data);

    while (true) {
        run(&app_data);
    }
}

esp_err_t boot(app_data_t *app_data) {
    if(!app_data) {
        return ESP_ERR_INVALID_ARG;
    }
    ADS112C04_t *adcs = app_data->adcs;
    uint8_t *num_adcs = &app_data->num_adcs;
    i2c_master_bus_handle_t *bus_handle = &app_data->bus_handle;

    setup_i2c(bus_handle, adcs, *num_adcs);

    return ESP_OK;
}

void run(app_data_t *app_data) {
    if(!app_data) {
        return ESP_ERR_INVALID_ARG;
    }
    ADS112C04_t *adcs = app_data->adcs;
}

esp_err_t setup_i2c(i2c_master_bus_handle_t *bus_handle,
                    ADS112C04_t *devices[],
                    size_t numDevices) {
    static const char *TAG = "SETUP_I2C";

    if (!bus_handle || !devices) {
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
    ESP_RETURN_ON_ERROR(i2c_new_master_bus(&i2c_mst_config, bus_handle), TAG,
                        "Failed to create i2c bus");

    // Set up devices on bus
    for (size_t i = 0; i < numDevices; i++) {
        ESP_RETURN_ON_ERROR(ADS112C04_init_i2c(devices[i], bus_handle), TAG,
                            "Failed to initialize i2c device");
    }
    return ESP_OK;
}