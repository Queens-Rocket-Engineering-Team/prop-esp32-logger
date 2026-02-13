#include "setup.h"
#include "ADS112C04.h"
#include <driver/i2c_master.h>
#include <esp_err.h>
#include <esp_wifi.h>
#include <nvs_flash.h>

static esp_err_t i2c_master_bus_scan(i2c_master_bus_handle_t bus_handle,
                                     uint8_t address[],
                                     size_t address_len);

static esp_err_t setup_i2c(i2c_master_bus_handle_t *bus_handle,
                           ADS112C04_t devices[],
                           size_t *num_devices);

void boot(app_data_t *app_data) {
    if (!app_data) {
        abort();
    }

    // Initialize NVS for wifi
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
        ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_ERROR_CHECK(esp_netif_init()); // Initialize NETIF for tcp
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    app_data->netif_handle = esp_netif_create_default_wifi_sta();

    wifi_init_config_t wifi_init_config = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&wifi_init_config));

    ESP_ERROR_CHECK(wifi_event_handler_register());

    wifi_config_t wifi_config = {
        .sta =
            {
                .ssid = CONFIG_ESP_WIFI_SSID,
                .password = CONFIG_ESP_WIFI_PASSWORD,
            },
    };

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start()); // Start wifi driver

    ESP_ERROR_CHECK(
        setup_i2c(&app_data->bus_handle, app_data->adcs, &app_data->num_adcs));
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
    // Initialize i2c master
    ret = i2c_new_master_bus(&i2c_mst_config, bus_handle);
    if (ret != ESP_OK) {
        return ret;
    }

    // Address array terminated with 0xFF, 112 possible addresses
    uint8_t device_addr[113];
    // Scan for all addresses on i2c bus
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
    // Non-reserved i2c addresses from 0x08 to 0x78
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