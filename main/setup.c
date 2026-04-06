#include "setup.h"
#include "ads112c04.h"
#include "config_json.h"
#include "wifi_tools.h"
#include <driver/i2c_master.h>
#include <esp_check.h>
#include <esp_err.h>
#include <esp_wifi.h>
#include <nvs_flash.h>

static const char *TAG = "SETUP";

static esp_err_t s_i2c_master_bus_scan(
    i2c_master_bus_handle_t bus_handle,
    uint8_t address[],
    size_t address_len
) {
    if (address == NULL || address_len == 0) {
        return ESP_ERR_INVALID_ARG;
    }

    size_t addr_index = 0;
    esp_err_t err;
    // non-reserved i2c addresses from 0x08 to 0x78
    for (uint8_t addr = 0x08; addr < 0x78; addr++) {
        err = i2c_master_probe(bus_handle, addr, 100);

        if (err == ESP_OK) {
            if (addr_index < address_len - 1) { // leave space for terminator
                address[addr_index++] = addr;
            } else {
                address[addr_index] = 0xFF;
                return ESP_ERR_NO_MEM;
            }
        } else if (err == ESP_ERR_NOT_FOUND) {
            continue;
        } else {
            address[addr_index] = 0xFF;
            return err;
        }
    }
    address[addr_index] = 0xFF;
    return ESP_OK;
}

static esp_err_t s_setup_i2c(
    i2c_master_bus_handle_t bus_handle,
    ads112c04_t adcs[]
) {

    if (adcs == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    // initialize bus
    i2c_master_bus_config_t i2c_mst_config = {
        .i2c_port = I2C_NUM_0,
        .scl_io_num = SCL_PIN,
        .sda_io_num = SDA_PIN,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = false, // external 1k pullups, dont need
    };

    // initialize i2c master
    ESP_RETURN_ON_ERROR(i2c_new_master_bus(&i2c_mst_config, &bus_handle), TAG, "Failed to create I2C bus");

    // address array terminated with 0xFF, 112 possible addresses
    uint8_t adc_addr[113];
    // scan for all addresses on i2c bus
    ESP_RETURN_ON_ERROR(s_i2c_master_bus_scan(bus_handle, adc_addr, sizeof(adc_addr)), TAG, "Failed to scan I2C bus");

    // set up devices on bus
    size_t i;
    for (i = 0; i < CONFIG_NUM_ADCS && adc_addr[i] != 0xFF; i++) {

        const ads112c04_config_t adc_cfg = {
            .addr = adc_addr[i],
            .bus_handle = bus_handle,
            .drdy_pin = adc_addr_to_drdy(adc_addr[i]),
        };

        ESP_RETURN_ON_ERROR(ads112c04_init(&adcs[i], &adc_cfg), TAG, "Failed to initialize ADS112C04");

    }
    return ESP_OK;
}

void app_setup(app_ctx_t *app_ctx) {
    if (!app_ctx) {
        abort();
    }

    ESP_ERROR_CHECK(
        s_setup_i2c(app_ctx->bus_handle, app_ctx->adcs)
    );
}

void network_setup(network_ctx_t *network_ctx) {
    if (!network_ctx) {
        abort();
    }

    // initialize NVS for wifi
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
        ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_ERROR_CHECK(esp_netif_init()); // initialize NETIF for tcp
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    network_ctx->netif_handle = esp_netif_create_default_wifi_sta();

    wifi_init_config_t wifi_init_config = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&wifi_init_config));

    ESP_ERROR_CHECK(wifi_event_handler_register(network_ctx));

    wifi_config_t wifi_config = {
        .sta = {
                .ssid = CONFIG_ESP_WIFI_SSID,
                .password = CONFIG_ESP_WIFI_PASSWORD,
                },
    };

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start()); // start wifi driver

    // set up the recv/send message queues
    static StaticQueue_t xStaticQueueSEND;
    static uint8_t ucQueueStorageAreaSEND[TCP_QUEUE_LEN * TCP_QUEUE_ITEM_SIZE];

    network_ctx->tcp_send_queue_handle = xQueueCreateStatic(
        TCP_QUEUE_LEN,
        TCP_QUEUE_ITEM_SIZE,
        ucQueueStorageAreaSEND,
        &xStaticQueueSEND
    );

    static StaticQueue_t xStaticQueueRECV;
    static uint8_t ucQueueStorageAreaRECV[TCP_QUEUE_LEN * TCP_QUEUE_ITEM_SIZE];

    network_ctx->tcp_recv_queue_handle = xQueueCreateStatic(
        TCP_QUEUE_LEN,
        TCP_QUEUE_ITEM_SIZE,
        ucQueueStorageAreaRECV,
        &xStaticQueueRECV
    );

    network_ctx->ssdp_sock = -1;
    network_ctx->server_sock = -1;
}
