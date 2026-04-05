#include "ads112c04.h"
#include "qret_protocol.h"
#include "setup.h"
#include "wifi_tools.h"
#include <esp_err.h>
#include <esp_log.h>
#include <esp_timer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/timers.h>
#include <netdb.h>
#include <stdio.h>

#define NETWORK_STACK_SIZE 32768

static const char *TAG = "MAIN";

void app_main(void) {

    esp_log_level_set("wifi", ESP_LOG_WARN);
    esp_log_level_set("phy_init", ESP_LOG_WARN);

    static network_ctx_t network_ctx = {0};
    network_ctx.server_port = TCP_SERVER_PORT;
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

    // processing for incoming/outgoing packets
    client_payload_t payload_in = {0};
    server_payload_t payload_out = {0};
    uint32_t ts_offset = 0;
    uint8_t sequence = 0;

    while (1) {

        xQueueReceive(
            network_ctx.tcp_recv_queue_handle, &payload_in, portMAX_DELAY
        );

        switch (payload_in.packet_type) {
        case PT_ESTOP:
            break;

        case PT_DISCOVERY:
            break;

        case PT_TIMESYNC: {
            ts_offset = payload_in.payload_data.header_only.ts_offset -
                        (uint32_t)(esp_timer_get_time() * 1000);

            payload_out.packet_type = PT_ACK;
            sequence++;
            const ack_packet_t ack = {
                .ack_packet_type = PT_TIMESYNC,
                .ack_sequence = payload_in.payload_data.header_only.sequence,
                .sequence = sequence,
                .ts_offset = ts_offset
            };
            payload_out.payload_data.ack = ack;

            xQueueSend(
                network_ctx.tcp_send_queue_handle, (void *)&payload_out, 0
            );
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
            sequence++;
            const ack_packet_t ack = {
                .ack_packet_type = PT_HEARTBEAT,
                .ack_sequence = payload_in.payload_data.header_only.sequence,
                .sequence = sequence,
                .ts_offset = ts_offset
            };
            payload_out.payload_data.ack = ack;

            xQueueSend(
                network_ctx.tcp_send_queue_handle, (void *)&payload_out, 0
            );
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