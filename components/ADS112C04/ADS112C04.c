#include "ADS112C04.h"
#include <driver/i2c_master.h>
#include <esp_err.h>

esp_err_t ADS112C04_init_i2c(ADS112C04_t *ADS112C04,
                             i2c_master_bus_handle_t *bus_handle) {
    if (!ADS112C04 || !bus_handle) {
        return ESP_ERR_INVALID_ARG;
    }       

    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = ADS112C04->address,
        .scl_speed_hz = 100000,
    };

    // Initialize device and fill dev_handle
    return i2c_master_bus_add_device(*bus_handle, &dev_cfg, &ADS112C04->dev_handle);
}

esp_err_t ADS112C04_write_register(ADS112C04_t *ADS112C04,
                                   uint8_t reg,
                                   uint8_t data) {
    // The WREG command is structured like: 0100 rrxx dddd dddd
    // Where rr is the register address and dddd dddd is the data to write.
    // This is sent in two parts: the command - 0100 rrxx, and the data - dddd
    // dddd

    uint8_t wreg =
        0x40 | ((reg & 0x03) << 2); // We know register only 0-3, so mask with
                                    // 0x03 then shift to correct position.

    return ESP_OK;
}

uint8_t ADS112C04_get_address(ADS112C04_t *ADS112C04) {
    return ADS112C04->address;
}

static esp_err_t _addressDevice(ADS112C04_t *ADS112C04, bool read) {
    // Address the specified ADS112 device for communication.
    // Addressing the devices constitutes sending a start condition, followed by
    // the address of the device being addressed. read: If True, address the
    // device for reading; otherwise, address for writing.
    if (!ADS112C04) {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t address_byte = (ADS112C04->address << 1) | (read ? 0x01 : 0x00);

    return ESP_OK;
}