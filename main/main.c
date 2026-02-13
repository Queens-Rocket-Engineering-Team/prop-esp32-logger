#include "ADS112C04.h"
#include "setup.h"
#include <esp_err.h>
#include <freertos/FreeRTOS.h>
#include <freertos/timers.h>
#include <stdio.h>
#include <tcp_tools.h>

#include "ssdp_tools.h"

void read_sensor_task(void *pvParameters);

void app_main(void) {

    static app_data_t app_data = {0};

    boot(&app_data);

    static StackType_t runStack_tcp_client[8192];
    static StaticTask_t runTCB_tcp_client;

    char server_ip[64];

    ssdp_discover_server(server_ip, sizeof server_ip, app_data.netif_handle);

    
    // xTaskCreateStaticPinnedToCore(tcp_client_recv_task, "TCP Event Loop",
    // 8192, NULL,
    //                               1, runStack_tcp_client, &runTCB_tcp_client,
    //                               PRO_CPU_NUM);

    // static StackType_t runStack_read_sensor[8192];
    // static StaticTask_t runTCB_read_sensor;

    // xTaskCreateStaticPinnedToCore(read_sensor_task, "Temp read", 8192,
    //                               &app_data, 1, runStack_read_sensor,
    //                               &runTCB_read_sensor, APP_CPU_NUM);
}

void read_sensor_task(void *pvParameters) {
    app_data_t *app_data = (app_data_t *)pvParameters;
    if (!app_data) {
        vTaskDelete(NULL);
    }
    ADS112C04_t *adcs = app_data->adcs;
    esp_err_t ret;

    set_continuous_mode(&adcs[0]);
    while (true) {

        float temperature;
        ret = ADS112C04_get_internal_temperature(&adcs[0], &temperature);
        printf("%f\n", temperature);

        vTaskDelay(pdMS_TO_TICKS(500));
    }
}