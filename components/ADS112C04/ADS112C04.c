#include "ADS112C04.h"
#include <driver/i2c_master.h>
#include <esp_err.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/timers.h>

#include <stdio.h>

#define ADS112C04_I2C_TIMEOUT 25

static const char *TAG = "ADS112C04";
// 0xFF terminator
static const uint8_t valid_pga_gain[] = {1, 2, 4, 8, 16, 32, 64, 128, 0xFF};

esp_err_t ADS112C04_init_i2c(ADS112C04_t *ADS112C04,
                             i2c_master_bus_handle_t *bus_handle) {
    if (!ADS112C04 || !bus_handle) {
        ESP_LOGE(TAG, "Invalid ARG to init");
        return ESP_ERR_INVALID_ARG;
    }

    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = ADS112C04->_address,
        .scl_speed_hz = 100000,
    };

    // Initialize device and fill dev_handle
    esp_err_t ret;
    ret = i2c_master_bus_add_device(*bus_handle, &dev_cfg,
                                    &ADS112C04->_dev_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Add device to master bus failed");
        return ret;
    }
    ADS112C04->_pgaGain = 1;
    ADS112C04->_pga_enabled = true;
    ADS112C04->_continuous_mode = false;
    return ESP_OK;
}

esp_err_t ADS112C04_set_address(ADS112C04_t *ADS112C04, uint8_t addr) {
    if (!ADS112C04) {
        ESP_LOGE(TAG, "Invalid ADC pointer");
        return ESP_ERR_INVALID_ARG;
    }
    ADS112C04->_address = addr;
    return ESP_OK;
}

static esp_err_t _ADS112C04_write_register(ADS112C04_t *ADS112C04,
                                           uint8_t reg,
                                           uint8_t write_byte) {
    // The WREG command is structured like: 0100 rrxx dddd dddd
    // Where rr is the register address and dddd dddd is the data to write.
    // This is sent in two parts: the command - 0100 rrxx, and
    // the data - dddddddd

    if (reg > 3) {
        ESP_LOGE(TAG, "Invalid registry");
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t wreg_cmd = 0x40 | (reg << 2);
    uint8_t wreg[] = {wreg_cmd, write_byte};

    esp_err_t ret;
    ret = i2c_master_transmit(ADS112C04->_dev_handle, wreg, 2,
                              ADS112C04_I2C_TIMEOUT);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Transmit wreg cmd failed");
        return ret;
    }
    return ESP_OK;
}

static esp_err_t _ADS112C04_read_register(ADS112C04_t *ADS112C04,
                                          uint8_t reg,
                                          uint8_t *reg_byte) {
    // The RREG command is structured like: 0010 rrxx
    // The full rreg sequence takes two i2c transactions:
    // 1. Send the rreg command to the device.
    // 2. Read the response data from the device.

    if (reg > 3) {
        ESP_LOGE(TAG, "Invalid registry");
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t rreg = 0x20 | (reg << 2);

    esp_err_t ret;
    ret = i2c_master_transmit(ADS112C04->_dev_handle, &rreg, 1,
                              ADS112C04_I2C_TIMEOUT);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Transmit rreg cmd failed");
        return ret;
    }
    ret = i2c_master_receive(ADS112C04->_dev_handle, reg_byte, 1,
                             ADS112C04_I2C_TIMEOUT);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Recieve registry byte failed");
        return ret;
    }
    return ESP_OK;
}

static esp_err_t ADS112C04_start_sync(ADS112C04_t *ADS112C04) {

    if (!ADS112C04) {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t start_sync_cmd = 0x08;
    esp_err_t ret;
    ret = i2c_master_transmit(ADS112C04->_dev_handle, &start_sync_cmd, 1,
                              ADS112C04_I2C_TIMEOUT);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to send start/sync");
        return ret;
    }
    return ESP_OK;
}

esp_err_t set_continuous_mode(ADS112C04_t *ADS112C04) {

    if (ADS112C04->_continuous_mode == true) {
        return ESP_OK;
    }

    uint8_t reg1;
    uint8_t CONT_MASK = 0x08;
    esp_err_t ret;
    ret = _ADS112C04_read_register(ADS112C04, 1, &reg1);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read reg1");
        return ret;
    }
    reg1 |= CONT_MASK;
    ret = _ADS112C04_write_register(ADS112C04, 1, reg1);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to write reg1");
        return ret;
    }
    ret = ADS112C04_start_sync(ADS112C04);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Continuous mode start/sync failed");
        return ret;
    }
    ADS112C04->_continuous_mode = true;
    return ESP_OK;
}

esp_err_t set_single_shot_mode(ADS112C04_t *ADS112C04) {
    if (ADS112C04->_continuous_mode == false) {
        return ESP_OK;
    }

    uint8_t reg1;
    uint8_t CONT_MASK = 0x08;
    esp_err_t ret;
    ret = _ADS112C04_read_register(ADS112C04, 1, &reg1);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read reg1");
        return ret;
    }
    reg1 &= ~CONT_MASK;
    ret = _ADS112C04_write_register(ADS112C04, 1, reg1);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to write reg1");
        return ret;
    }
    ADS112C04->_continuous_mode = false;
    return ESP_OK;
}

static esp_err_t _disable_pga(ADS112C04_t *ADS112C04) {

    if (ADS112C04->_pga_enabled == false) {
        return ESP_OK;
    }

    uint8_t reg0;
    uint8_t PGA_MASK = 0x01;
    esp_err_t ret;

    ret = _ADS112C04_read_register(ADS112C04, 0, &reg0);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read reg0");
        return ret;
    }
    reg0 |= PGA_MASK; // Disable PGA
    ret = _ADS112C04_write_register(ADS112C04, 0, reg0);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to write reg0");
        return ret;
    }
    ADS112C04->_pga_enabled = false;
    return ESP_OK;
}

static uint8_t _lin_search(uint8_t val, const uint8_t arr[]) {
    for (uint8_t i = 0; arr[i] != 0xFF; i++) {
        if (val == arr[i]) {
            return i;
        }
    }
    return 0xFF;
}

static esp_err_t _set_pga(ADS112C04_t *ADS112C04, uint8_t pga_gain) {

    if (ADS112C04->_pgaGain == pga_gain) {
        return ESP_OK;
    }

    uint8_t gain_bits;
    gain_bits = _lin_search(pga_gain, valid_pga_gain);
    if (gain_bits == 0xFF) {
        ESP_LOGE(TAG, "Given PGA gain is not a valid gain");
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t reg0;
    uint8_t PGA_MASK = 0x01;
    uint8_t GAIN_MASK = 0x0E;
    esp_err_t ret;

    ret = _ADS112C04_read_register(ADS112C04, 0, &reg0);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read reg0");
        return ret;
    }
    reg0 &= ~PGA_MASK; // enable PGA
    reg0 &= ~GAIN_MASK;
    reg0 |= ((gain_bits << 1) & GAIN_MASK); // Set gain value
    ret = _ADS112C04_write_register(ADS112C04, 0, reg0);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to write reg0");
        return ret;
    }
    ADS112C04->_pgaGain = pga_gain;
    ADS112C04->_pga_enabled = true;
    return ESP_OK;
}

static float _bits_to_temperature(uint8_t msb, uint8_t lsb) {

    uint16_t raw = (msb << 8) | lsb; // Combine MSB and LSB
    raw = raw >> 2;                  // 14 bit left-aligned reading
    if (raw & 0x2000) {              // If the sign bit is set
        raw -= 1 << 14;              // Convert to negative value
    }
    float temperature = (float) raw / 0x20; // Divide for temp resolution
    return temperature;
}

esp_err_t ADS112C04_get_internal_temperature(ADS112C04_t *ADS112C04,
                                             float *temperature) {

    if (!ADS112C04 || !temperature) {
        ESP_LOGE(TAG, "Invalid arg to get_internal_temperature");
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t reg1;
    uint8_t TEMP_SENSE_MASK = 0x01;
    esp_err_t ret;

    // Enable temperature sensor
    ret = _ADS112C04_read_register(ADS112C04, 1, &reg1);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read reg1 for temp reading");
        return ret;
    }
    reg1 |= TEMP_SENSE_MASK;
    ret = _ADS112C04_write_register(ADS112C04, 1, reg1);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to write enable temp to reg1");
        return ret;
    }

    vTaskDelay(pdMS_TO_TICKS(100)); // Delay 100 ms

    // Read data from ADC
    uint8_t rdata = 0x10;
    ret = i2c_master_transmit(ADS112C04->_dev_handle, &rdata, 1,
                              ADS112C04_I2C_TIMEOUT);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to transmit rdata cmd for temp reading");
        return ret;
    }
    uint8_t reading[2] = {0};
    ret = i2c_master_receive(ADS112C04->_dev_handle, reading, 2,
                             ADS112C04_I2C_TIMEOUT);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to recieve temp reading");
        return ret;
    }
    printf("%d %d\n", reading[0], reading[1]);
    
    // Disable temperature sensor
    reg1 &= ~TEMP_SENSE_MASK;
    ret = _ADS112C04_write_register(ADS112C04, 1, reg1);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to write disable temp to reg1");
        return ret;
    }

    *temperature = _bits_to_temperature(reading[0], reading[1]);
    return ESP_OK;
}