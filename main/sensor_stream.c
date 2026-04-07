#include <esp_err.h>
#include <esp_log.h>
#include <esp_timer.h>
#include <freertos/FreeRTOS.h>

#include "ads112c04.h"
#include "current_sensor.h"
#include "load_cell.h"
#include "pressure_transducer.h"
#include "resistance_sensor.h"
#include "thermocouple.h"

#include "config_json.h"
#include "sensor_stream.h"
#include "setup.h"

static const char *TAG = "SENSOR STREAM";

void sensor_stream(void *pvParams) {
    app_ctx_t *app_ctx = (app_ctx_t *)pvParams;

    esp_err_t err;

    uint16_t frequency = 100; // default 10Hz

    while (1) {

        xEventGroupWaitBits(
            app_ctx->sensor_stream_event_group_handle,
            SENSOR_STREAM_ENABLE_BIT | SENSORS_SINGLE_READING_BIT,
            pdFALSE,
            pdFALSE,
            portMAX_DELAY
        );

        uint32_t new_frequency;
        if (xTaskNotifyWait(0, 0, &new_frequency, 0) == pdTRUE) {
            if (new_frequency != 0) {
                frequency = (uint16_t)new_frequency;
                ESP_LOGI(TAG, "Stream frequency: %u", frequency);
            }
        }


        static qret_sensor_data data[CONFIG_NUM_SENSORS] = {0};

        for (size_t i = 0; i < CONFIG_NUM_SENSORS; i++) {

            data[i].sensor_id = i;

            switch (app_ctx->sensors[i].sensor_type) {
            case THERMOCOUPLE:
                switch (app_ctx->sensors[i].sensor.thermocouple.unit) {
                case THERMOCOUPLE_C:
                    data[i].unit = UNIT_CELSIUS;
                    break;
                case THERMOCOUPLE_K:
                    data[i].unit = UNIT_KELVIN;
                    break;
                case THERMOCOUPLE_F:
                    data[i].unit = UNIT_FAHRENHEIT;
                    break;
                }
                err = get_thermocouple_reading(&app_ctx->sensors[i].sensor.thermocouple, &data[i].value);
                if (err != ESP_OK) {
                    ESP_LOGE(TAG, "Failed to get thermocouple reading: %s", esp_err_to_name(err));
                }
                break;
            case PRESSURE_TRANSDUCER:
                switch (app_ctx->sensors[i].sensor.pressure_transducer.unit) {
                case PRESSURE_TRANSDUCER_PSI:
                    data[i].unit = UNIT_PSI;
                    break;
                case PRESSURE_TRANSDUCER_BAR:
                    data[i].unit = UNIT_BAR;
                    break;
                case PRESSURE_TRANSDUCER_PA:
                    data[i].unit = UNIT_PASCAL;
                    break;
                }
                err = get_pressure_reading(&app_ctx->sensors[i].sensor.pressure_transducer, &data[i].value);
                if (err != ESP_OK) {
                    ESP_LOGE(TAG, "Failed to get pressure transducer reading: %s", esp_err_to_name(err));
                }
                break;
            case LOAD_CELL:
                switch (app_ctx->sensors[i].sensor.load_cell.unit) {
                case LOAD_CELL_KG:
                    data[i].unit = UNIT_KILOGRAMS;
                    break;
                case LOAD_CELL_N:
                    data[i].unit = UNIT_NEWTONS;
                    break;
                }
                err = get_load_cell_reading(&app_ctx->sensors[i].sensor.load_cell, &data[i].value);
                if (err != ESP_OK) {
                    ESP_LOGE(TAG, "Failed to get load cell reading: %s", esp_err_to_name(err));
                }
                break;
            case RESISTANCE_SENSOR:
                switch (app_ctx->sensors[i].sensor.resistance_sensor.unit) {
                case RESISTANCE_SENSOR_OHMS:
                    data[i].unit = UNIT_OHMS;
                    break;
                }
                err = get_resistance_reading(&app_ctx->sensors[i].sensor.resistance_sensor, &data[i].value);
                if (err != ESP_OK) {
                    ESP_LOGE(TAG, "Failed to get resistance reading: %s", esp_err_to_name(err));
                }
                break;
            case CURRENT_SENSOR:
                switch (app_ctx->sensors[i].sensor.current_sensor.unit) {
                case CURRENT_SENSOR_A:
                    data[i].unit = UNIT_AMPS;
                    break;
                }
                err = get_current_reading(&app_ctx->sensors[i].sensor.current_sensor, &data[i].value);
                if (err != ESP_OK) {
                    ESP_LOGE(TAG, "Failed to get current reading: %s", esp_err_to_name(err));
                }
                break;
            }
        }

        const uint32_t timestamp = app_ctx->ts_offset + (uint32_t)(esp_timer_get_time() / 1000);
        taskENTER_CRITICAL(&app_ctx->sequence_spinlock);
        const uint16_t sequence = ++app_ctx->sequence;
        taskEXIT_CRITICAL(&app_ctx->sequence_spinlock);
        qret_data_packet data_packet = {
            .sensor_data = data,
            .sensor_count = CONFIG_NUM_SENSORS,
            .header = {
                       .sequence = sequence,
                       .timestamp = timestamp,
                       },
        };

        xQueueSend(app_ctx->network_ctx->udp_send_queue_handle, (void *)&data_packet, MESSAGE_QUEUE_TIMEOUT);

        if (xEventGroupGetBits(app_ctx->sensor_stream_event_group_handle) & SENSORS_SINGLE_READING_BIT) {
            xEventGroupClearBits(app_ctx->sensor_stream_event_group_handle, SENSORS_SINGLE_READING_BIT);
        } else {
            vTaskDelay(pdMS_TO_TICKS((uint32_t)1000 / frequency));
        }
    }
}
