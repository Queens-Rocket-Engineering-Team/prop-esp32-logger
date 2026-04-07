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
    client_payload_t payload_in = {0};
    server_payload_t payload_out = {0};

    while (1) {

        xQueueReceive(network_ctx.tcp_recv_queue_handle, &payload_in, portMAX_DELAY);

        switch (payload_in.packet_type) {
        case PT_ESTOP: {

        } break;

        case PT_TIMESYNC: {
            app_ctx.ts_offset = payload_in.payload_data.header_only.ts_offset - (uint32_t)(esp_timer_get_time() / 1000);

            payload_out.packet_type = PT_ACK;
            taskENTER_CRITICAL(&app_ctx.sequence_spinlock);
            const ack_packet_t ack = {
                .ack_packet_type = PT_TIMESYNC,
                .ack_sequence = payload_in.payload_data.header_only.sequence,
                .sequence = ++app_ctx.sequence,
                .ts_offset = app_ctx.ts_offset + (uint32_t)(esp_timer_get_time() / 1000),
            };
            taskEXIT_CRITICAL(&app_ctx.sequence_spinlock);
            payload_out.payload_data.ack = ack;

            xQueueSend(network_ctx.tcp_send_queue_handle, (void *)&payload_out, 0);
        } break;

        case PT_CONTROL: {

        } break;

        case PT_STATUS_REQUEST: {

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
            taskENTER_CRITICAL(&app_ctx.sequence_spinlock);
            const ack_packet_t ack = {
                .ack_packet_type = PT_STREAM_START,
                .ack_sequence = payload_in.payload_data.header_only.sequence,
                .sequence = ++app_ctx.sequence,
                .ts_offset = app_ctx.ts_offset + (uint32_t)(esp_timer_get_time() / 1000),
            };
            taskEXIT_CRITICAL(&app_ctx.sequence_spinlock);
            payload_out.payload_data.ack = ack;

            xQueueSend(network_ctx.tcp_send_queue_handle, (void *)&payload_out, 0);
        } break;

        case PT_STREAM_STOP: {
            xEventGroupClearBits(app_ctx.sensor_stream_event_group_handle, SENSOR_STREAM_ENABLE_BIT);
            ESP_LOGI(TAG, "Sensor stream stopped");

            payload_out.packet_type = PT_ACK;
            taskENTER_CRITICAL(&app_ctx.sequence_spinlock);
            const ack_packet_t ack = {
                .ack_packet_type = PT_STREAM_STOP,
                .ack_sequence = payload_in.payload_data.header_only.sequence,
                .sequence = ++app_ctx.sequence,
                .ts_offset = app_ctx.ts_offset + (uint32_t)(esp_timer_get_time() / 1000),
            };
            taskEXIT_CRITICAL(&app_ctx.sequence_spinlock);
            payload_out.payload_data.ack = ack;

            xQueueSend(network_ctx.tcp_send_queue_handle, (void *)&payload_out, 0);
        } break;

        case PT_GET_SINGLE: {
            
        } break;

        case PT_HEARTBEAT: {
            payload_out.packet_type = PT_ACK;
            taskENTER_CRITICAL(&app_ctx.sequence_spinlock);
            const ack_packet_t ack = {
                .ack_packet_type = PT_HEARTBEAT,
                .ack_sequence = payload_in.payload_data.header_only.sequence,
                .sequence = ++app_ctx.sequence,
                .ts_offset = app_ctx.ts_offset + (uint32_t)(esp_timer_get_time() / 1000),
            };
            taskEXIT_CRITICAL(&app_ctx.sequence_spinlock);
            payload_out.payload_data.ack = ack;

            xQueueSend(network_ctx.tcp_send_queue_handle, (void *)&payload_out, 0);
        } break;

        default:
            ESP_LOGE(TAG, "Invalid client packet type recieved");
        }
    }
}
