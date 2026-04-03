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

const char *TAG = "Main";

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

    static StackType_t runStack_tcp_client[8192];
    static StaticTask_t runTCB_tcp_client;

    // xTaskCreateStaticPinnedToCore(tcp_client_recv_task, "TCP RECV Event
    // Loop", 8192, NULL,
    //                               1, runStack_tcp_client, &runTCB_tcp_client,
    //                               PRO_CPU_NUM);

    // static StackType_t runStack_read_sensor[8192];
    // static StaticTask_t runTCB_read_sensor;

    // xTaskCreateStaticPinnedToCore(read_sensor_task, "Temp read", 8192,
    //                               &app_data, 1, runStack_read_sensor,
    //                               &runTCB_read_sensor, APP_CPU_NUM);
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