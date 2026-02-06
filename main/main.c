#include "ADS112C04.h"
#include "setup.h"
#include <esp_err.h>
#include <freertos/FreeRTOS.h>
#include <freertos/timers.h>
#include <stdio.h>

void run(void *pvParameters);

void app_main(void) {

    static app_data_t app_data = {};
    
    boot(&app_data);

    static StackType_t runStack[8192];
    static StaticTask_t runTCB;
    xTaskCreateStatic(run, "Temp read", 8192, &app_data, 1, runStack, &runTCB);
}

void run(void *pvParameters) {
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