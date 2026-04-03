#include "ADS112C04.h"
#include "setup.h"
#include "wifi_tools.h"
#include <esp_err.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/timers.h>
#include <netdb.h>
#include <stdio.h>

#define NETWORK_STACK_SIZE 16384

// void read_sensor_task(void *pvParameters);

static const char *TAG = "Main";

void app_main(void) {

    esp_log_level_set("wifi", ESP_LOG_WARN);
    esp_log_level_set("phy_init", ESP_LOG_WARN);

    static network_ctx_t network_ctx = {0};
    network_ctx.server_port = 50000;
    static app_ctx_t app_ctx = {0};

    network_setup(&network_ctx);
    app_setup(&app_ctx);
    ESP_LOGI(TAG, "Setup complete");

    static StaticTask_t xTaskBufferNetwork;
    static StackType_t xStackNetwork[NETWORK_STACK_SIZE];

    network_ctx.network_manager_handle = xTaskCreateStatic(
        network_state_manager,
        "Network Manager",
        NETWORK_STACK_SIZE,
        (void *)&network_ctx,
        2,
        xStackNetwork,
        &xTaskBufferNetwork
    );

    
    
}

// void read_sensor_task(void *pvParameters) {
//     app_data_t *app_data = (app_data_t *)pvParameters;
//     if (!app_data) {
//         vTaskDelete(NULL);
//     }
//     ADS112C04_t *adcs = app_data->adcs;
//     esp_err_t ret;

//     set_continuous_mode(&adcs[0]);
//     while (true) {

//         float temperature;
//         ret = ADS112C04_get_internal_temperature(&adcs[0], &temperature);
//         printf("%f\n", temperature);

//         vTaskDelay(pdMS_TO_TICKS(500));
//     }
// }