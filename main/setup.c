#include "setup.h"
#include "ADS112C04.h"
#include "wifi_tools.h"
#include <driver/i2c_master.h>
#include <esp_err.h>
#include <esp_wifi.h>
#include <nvs_flash.h>

static esp_err_t i2c_master_bus_scan(
    i2c_master_bus_handle_t bus_handle,
    uint8_t address[],
    size_t address_len
);

static esp_err_t setup_i2c(
    i2c_master_bus_handle_t *bus_handle,
    ADS112C04_t devices[],
    size_t *num_devices
);

void app_setup(app_ctx_t *app_ctx) {
    if (!app_ctx) {
        abort();
    }

    ESP_ERROR_CHECK(
        setup_i2c(&app_ctx->bus_handle, app_ctx->adcs, &app_ctx->num_adcs)
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

static esp_err_t setup_i2c(
    i2c_master_bus_handle_t *bus_handle,
    ADS112C04_t devices[],
    size_t *num_devices
) {

    if (bus_handle == NULL || devices == NULL || num_devices == NULL) {
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

    esp_err_t ret;
    // initialize i2c master
    ret = i2c_new_master_bus(&i2c_mst_config, bus_handle);
    if (ret != ESP_OK) {
        return ret;
    }

    // address array terminated with 0xFF, 112 possible addresses
    uint8_t device_addr[113];
    // scan for all addresses on i2c bus
    ret = i2c_master_bus_scan(*bus_handle, device_addr, 113);
    if (ret != ESP_OK) {
        return ret;
    }

    // set up devices on bus
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

static esp_err_t i2c_master_bus_scan(
    i2c_master_bus_handle_t bus_handle,
    uint8_t address[],
    size_t address_len
) {
    if (!address || address_len == 0) {
        return ESP_ERR_INVALID_ARG;
    }

    size_t addr_index = 0;
    esp_err_t ret;
    // non-reserved i2c addresses from 0x08 to 0x78
    for (uint8_t addr = 0x08; addr < 0x78; addr++) {
        ret = i2c_master_probe(bus_handle, addr, 100);

        if (ret == ESP_OK) {
            if (addr_index < address_len - 1) { // leave space for terminator
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