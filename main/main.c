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
#include "sensor_stream.h"
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
    qret_client_payload payload_in = {0};
    qret_server_payload payload_out = {0};

    while (1) {

        xQueueReceive(network_ctx.tcp_recv_queue_handle, &payload_in, portMAX_DELAY);

        switch (payload_in.packet_type) {
        case PT_ESTOP: {
            for (size_t i = 0; i < CONFIG_NUM_CONTROLS; i++) {
                control_set_default(&app_ctx.controls[i]);
            }
            xEventGroupClearBits(app_ctx.sensor_stream_event_group_handle, SENSOR_STREAM_ENABLE_BIT);
        }
            continue;

        case PT_TIMESYNC: {
            app_ctx.ts_offset = payload_in.payload_data.header_only.ts_offset - (uint32_t)(esp_timer_get_time() / 1000);
            payload_out.packet_type = PT_ACK;
            const uint32_t timestamp = app_ctx.ts_offset + (uint32_t)(esp_timer_get_time() / 1000);
            taskENTER_CRITICAL(&app_ctx.sequence_spinlock);
            const qret_ack_packet ack = {
                .ack_packet_type = PT_TIMESYNC,
                .ack_sequence = payload_in.payload_data.header_only.sequence,
                .sequence = ++app_ctx.sequence,
                .ts_offset = timestamp,
            };
            taskEXIT_CRITICAL(&app_ctx.sequence_spinlock);
            payload_out.payload_data.ack = ack;
        } break;

        case PT_CONTROL: {
            uint8_t i = payload_in.payload_data.control.command_id;
            esp_err_t err = ESP_FAIL;
            qret_packet_err nack_error_code = ERR_HARDWARE_FAULT;

            if (i < CONFIG_NUM_CONTROLS) {
                if (payload_in.payload_data.control.command_state == CS_OPEN) {
                    err = control_open(&app_ctx.controls[i]);
                } else if (payload_in.payload_data.control.command_state == CS_CLOSED) {
                    err = control_close(&app_ctx.controls[i]);
                }
            } else {
                nack_error_code = ERR_INVALID_PARAM;
            }
            
            ESP_ERROR_CHECK_WITHOUT_ABORT(err);
            if (err == ESP_OK) {
                payload_out.packet_type = PT_ACK;
                const uint32_t timestamp = app_ctx.ts_offset + (uint32_t)(esp_timer_get_time() / 1000);
                taskENTER_CRITICAL(&app_ctx.sequence_spinlock);
                const qret_ack_packet ack = {
                    .ack_packet_type = PT_CONTROL,
                    .ack_sequence = payload_in.payload_data.header_only.sequence,
                    .sequence = ++app_ctx.sequence,
                    .ts_offset = timestamp,
                };
                taskEXIT_CRITICAL(&app_ctx.sequence_spinlock);
                payload_out.payload_data.ack = ack;
            } else {
                payload_out.packet_type = PT_NACK;
                const uint32_t timestamp = app_ctx.ts_offset + (uint32_t)(esp_timer_get_time() / 1000);
                taskENTER_CRITICAL(&app_ctx.sequence_spinlock);
                const qret_nack_packet nack = {
                    .nack_packet_type = PT_CONTROL,
                    .nack_sequence = payload_in.payload_data.header_only.sequence,
                    .sequence = ++app_ctx.sequence,
                    .ts_offset = timestamp,
                    .nack_error_code = nack_error_code
                };
                taskEXIT_CRITICAL(&app_ctx.sequence_spinlock);
                payload_out.payload_data.nack = nack;
            }
        } break;

        case PT_STATUS_REQUEST: {
            static qret_control_status control_status[CONFIG_NUM_CONTROLS] = {0};

            for (size_t i = 0; i < CONFIG_NUM_CONTROLS; i++) {
                control_status[i].control_id = i;
                control_status[i].control_state = control_get_state(&app_ctx.controls[i]);
            }

            payload_out.packet_type = PT_STATUS;
            const uint32_t timestamp = app_ctx.ts_offset + (uint32_t)(esp_timer_get_time() / 1000);
            taskENTER_CRITICAL(&app_ctx.sequence_spinlock);
            const qret_status_packet status = {
                .control_data = control_status,
                .control_count = CONFIG_NUM_CONTROLS,
                .device_status = DS_ACTIVE,
                .sequence = ++app_ctx.sequence,
                .ts_offset = timestamp,

            };
            taskEXIT_CRITICAL(&app_ctx.sequence_spinlock);
            payload_out.payload_data.status = status;
        } break;

        case PT_STREAM_START: {
            // give the stream task the frequency
            xTaskNotify(
                app_ctx.sensor_stream_handle,
                payload_in.payload_data.stream_start.stream_frequency,
                eSetValueWithOverwrite
            );
            // notify the stream task to start
            xEventGroupSetBits(app_ctx.sensor_stream_event_group_handle, SENSOR_STREAM_ENABLE_BIT);
            ESP_LOGI(TAG, "Sensor stream started");

            payload_out.packet_type = PT_ACK;
            const uint32_t timestamp = app_ctx.ts_offset + (uint32_t)(esp_timer_get_time() / 1000);
            taskENTER_CRITICAL(&app_ctx.sequence_spinlock);
            const qret_ack_packet ack = {
                .ack_packet_type = PT_STREAM_START,
                .ack_sequence = payload_in.payload_data.header_only.sequence,
                .sequence = ++app_ctx.sequence,
                .ts_offset = timestamp,
            };
            taskEXIT_CRITICAL(&app_ctx.sequence_spinlock);
            payload_out.payload_data.ack = ack;
        } break;

        case PT_STREAM_STOP: {
            xEventGroupClearBits(app_ctx.sensor_stream_event_group_handle, SENSOR_STREAM_ENABLE_BIT);
            ESP_LOGI(TAG, "Sensor stream stopped");

            payload_out.packet_type = PT_ACK;
            const uint32_t timestamp = app_ctx.ts_offset + (uint32_t)(esp_timer_get_time() / 1000);
            taskENTER_CRITICAL(&app_ctx.sequence_spinlock);
            const qret_ack_packet ack = {
                .ack_packet_type = PT_STREAM_STOP,
                .ack_sequence = payload_in.payload_data.header_only.sequence,
                .sequence = ++app_ctx.sequence,
                .ts_offset = timestamp,
            };
            taskEXIT_CRITICAL(&app_ctx.sequence_spinlock);
            payload_out.payload_data.ack = ack;
        } break;

        case PT_GET_SINGLE: {
            // notify the stream task to send single reading
            xEventGroupSetBits(app_ctx.sensor_stream_event_group_handle, SENSORS_SINGLE_READING_BIT);
            ESP_LOGI(TAG, "Sensors single reading");
        }
            continue;

        case PT_HEARTBEAT: {
            payload_out.packet_type = PT_ACK;
            const uint32_t timestamp = app_ctx.ts_offset + (uint32_t)(esp_timer_get_time() / 1000);
            taskENTER_CRITICAL(&app_ctx.sequence_spinlock);
            const qret_ack_packet ack = {
                .ack_packet_type = PT_HEARTBEAT,
                .ack_sequence = payload_in.payload_data.header_only.sequence,
                .sequence = ++app_ctx.sequence,
                .ts_offset = timestamp,
            };
            taskEXIT_CRITICAL(&app_ctx.sequence_spinlock);
            payload_out.payload_data.ack = ack;
        } break;

        case PT_ACK:
        case PT_NACK:
            continue;

        default:
            ESP_LOGE(TAG, "Invalid client packet type recieved: %d", payload_in.packet_type);
            continue;
        }

        xQueueSend(network_ctx.tcp_send_queue_handle, (void *)&payload_out, MESSAGE_QUEUE_TIMEOUT);
    }
}
