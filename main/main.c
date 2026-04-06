#include <esp_err.h>
#include <esp_log.h>
#include <esp_timer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/timers.h>
#include <netdb.h>
#include <stdio.h>

#include "ads112c04.h"
#include "current_sensor.h"
#include "load_cell.h"
#include "pressure_transducer.h"
#include "resistance_sensor.h"
#include "thermocouple.h"

#include "qret_protocol.h"
#include "setup.h"
#include "wifi_tools.h"

static const char *TAG = "MAIN";

void app_main(void) {

    esp_log_level_set("wifi", ESP_LOG_WARN);
    esp_log_level_set("phy_init", ESP_LOG_WARN);

    static network_ctx_t network_ctx = {0};
    static app_ctx_t app_ctx = {0};

    ESP_ERROR_CHECK(app_setup(&app_ctx, &network_ctx));
    ESP_LOGI(TAG, "Setup complete");

    // processing for incoming/outgoing packets
    client_payload_t payload_in = {0};
    server_payload_t payload_out = {0};

    while (1) {

        xQueueReceive(network_ctx.tcp_recv_queue_handle, &payload_in, portMAX_DELAY);

        switch (payload_in.packet_type) {
        case PT_ESTOP:
            break;

        case PT_DISCOVERY:
            break;

        case PT_TIMESYNC: {
            payload_out.packet_type = PT_ACK;
            taskENTER_CRITICAL(&app_ctx.sequence_spinlock);
            app_ctx.ts_offset = payload_in.payload_data.header_only.ts_offset - (uint32_t)(esp_timer_get_time() / 1000);
            ;
            const ack_packet_t ack = {
                .ack_packet_type = PT_TIMESYNC,
                .ack_sequence = payload_in.payload_data.header_only.sequence,
                .sequence = ++app_ctx.sequence,
                .ts_offset = app_ctx.ts_offset
            };
            taskEXIT_CRITICAL(&app_ctx.sequence_spinlock);
            payload_out.payload_data.ack = ack;

            xQueueSend(network_ctx.tcp_send_queue_handle, (void *)&payload_out, 0);
        } break;

        case PT_CONTROL: {

        } break;

        case PT_STATUS_REQUEST:
            break;

        case PT_STREAM_START:
            break;

        case PT_STREAM_STOP:
            break;

        case PT_GET_SINGLE:
            break;

        case PT_HEARTBEAT: {
            payload_out.packet_type = PT_ACK;
            taskENTER_CRITICAL(&app_ctx.sequence_spinlock);
            const ack_packet_t ack = {
                .ack_packet_type = PT_HEARTBEAT,
                .ack_sequence = payload_in.payload_data.header_only.sequence,
                .sequence = ++app_ctx.sequence,
                .ts_offset = app_ctx.ts_offset
            };
            taskEXIT_CRITICAL(&app_ctx.sequence_spinlock);
            payload_out.payload_data.ack = ack;

            xQueueSend(network_ctx.tcp_send_queue_handle, (void *)&payload_out, 0);
        } break;

        case PT_ACK:
            break;

        case PT_NACK:
            break;

        default:
            ESP_LOGE(TAG, "Invalid client packet type recieved");
        }
    }
}

void sensor_stream_task(void *pvParams) {
    app_ctx_t *app_ctx = (app_ctx_t *)pvParams;

    while (1) {

        static protocol_sensor_data_t data[CONFIG_NUM_SENSORS] = {0};

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
                get_thermocouple_reading(&app_ctx->sensors[i].sensor.thermocouple, &data[i].value);
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
                get_pressure_reading(&app_ctx->sensors[i].sensor.pressure_transducer, &data[i].value);
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
                get_load_cell_reading(&app_ctx->sensors[i].sensor.load_cell, &data[i].value);
                break;
            case RESISTANCE_SENSOR:
                switch (app_ctx->sensors[i].sensor.resistance_sensor.unit) {
                case RESISTANCE_SENSOR_OHMS:
                    data[i].unit = UNIT_OHMS;
                    break;
                }
                get_resistance_reading(&app_ctx->sensors[i].sensor.resistance_sensor, &data[i].value);
                break;
            case CURRENT_SENSOR:
                switch (app_ctx->sensors[i].sensor.current_sensor.unit) {
                case CURRENT_SENSOR_A:
                    data[i].unit = UNIT_AMPS;
                    break;
                }
                get_current_reading(&app_ctx->sensors[i].sensor.current_sensor, &data[i].value);
                break;
            }
        }

        taskENTER_CRITICAL(&app_ctx->sequence_spinlock);
        data_packet_t data_packet = {
            .sensor_data = data,
            .sensor_count = CONFIG_NUM_SENSORS,
            .sequence = ++app_ctx->sequence,
            .ts_offset = app_ctx->ts_offset,
        };
        taskEXIT_CRITICAL(&app_ctx->sequence_spinlock);

        xQueueSend(app_ctx->network_ctx->udp_send_queue_handle, (void *)&data_packet, 0);
    }
}
