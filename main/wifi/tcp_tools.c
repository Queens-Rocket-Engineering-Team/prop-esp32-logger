#include "esp_config_json.h"
#include "qret_protocol.h"
#include "setup.h"
#include "wifi_tools.h"
#include <esp_err.h>
#include <esp_log.h>
#include <string.h>
#include <sys/socket.h>

#define RX_BUFFER_LEN 2048
#define TX_BUFFER_LEN 8192

static const char *TAG = "TCP";

esp_err_t tcp_connect_to_server(
    int32_t *sock,
    uint32_t server_ip,
    uint16_t server_port
) {

    if (sock == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t ret = ESP_FAIL;
    *sock = -1;

    int err;

    struct sockaddr_in dest_addr = {0};
    dest_addr.sin_addr.s_addr = server_ip;
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(server_port);

    // creating client's socket
    *sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (*sock < 0) {
        ret = ESP_FAIL;
        goto cleanup;
    }

    int enable = 1;
    err = setsockopt(*sock, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable));
    if (err != 0) {
        close(*sock);
        ret = ESP_FAIL;
        goto cleanup;
    }

    // attempt to connect to the server
    err = connect(*sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
    if (err != 0) {
        ESP_LOGE(TAG, "connect failed: errno %d", errno);
        close(*sock);
        goto cleanup;
    }
    ESP_LOGI(TAG, "Connected to server");
    ret = ESP_OK;

cleanup:
    return ret;
}

void tcp_client_recv(void *pvParams) {
    network_ctx_t *network_ctx = (network_ctx_t *)pvParams;

    if (network_ctx == NULL) {
        goto cleanup;
    }

    static uint8_t rx_buffer[RX_BUFFER_LEN] = {0};
    static uint16_t packet_data_len = 0;
    static client_payload_t payload = {0};

    uint32_t server_connected;
    xTaskNotify(
        xTaskGetCurrentTaskHandle(), SERVER_DISCONNECTED, eSetValueWithOverwrite
    );

    while (1) {
        // only pause when server is disconnected
        xTaskNotifyWait(
            0,
            0,
            &server_connected,
            (server_connected == SERVER_CONNECTED) ? 0 : portMAX_DELAY
        );

        if (server_connected == SERVER_CONNECTED) {

            // get the full header from a packet
            int32_t len = recv(
                network_ctx->server_sock, rx_buffer, HEADER_SIZE, MSG_WAITALL
            );
            if (len < 0) {
                ESP_LOGE(TAG, "recv failed: errno %d", errno);
                switch (errno) {
                case ECONNRESET:
                case EBADF:
                    continue;
                default:
                    goto cleanup;
                }
            }
            if (len == 0) {
                ESP_LOGI(TAG, "Connection closed by server gracefully");
                continue;
            }

            get_packet_len(rx_buffer, HEADER_SIZE, &packet_data_len);

            if (packet_data_len > 0) {
                if (sizeof(rx_buffer) - HEADER_SIZE < packet_data_len) {
                    ESP_LOGE(
                        TAG,
                        "rx_buffer ran out of space: packet len %d",
                        packet_data_len
                    );
                    break;
                }
                // recieve the rest of the packet if not header only
                len = recv(
                    network_ctx->server_sock,
                    rx_buffer + HEADER_SIZE,
                    packet_data_len,
                    MSG_WAITALL
                );
                if (len < 0) {
                    ESP_LOGE(TAG, "recv failed: errno %d", errno);
                    switch (errno) {
                    case ECONNRESET:
                    case EBADF:
                        continue;
                    default:
                        goto cleanup;
                    }
                }
                if (len == 0) {
                    ESP_LOGI(TAG, "Connection closed by server gracefully");
                    continue;
                }
            }

            ESP_LOGD(TAG, "Received %d bytes from server:", len);
            ESP_LOG_BUFFER_HEXDUMP(TAG, rx_buffer, len, ESP_LOG_DEBUG);

            // parse and send packet data for processing
            client_parse_packet(rx_buffer, sizeof(rx_buffer), &payload);
            xQueueSend(network_ctx->tcp_recv_queue_handle, (void *)&payload, 0);
        }
    }

    if (network_ctx->server_sock != -1) {
        goto cleanup;
    }

cleanup:
    shutdown(network_ctx->server_sock, 0);
    close(network_ctx->server_sock);
    // vTaskDelete(NULL);
}

void tcp_client_send(void *pvParams) {
    network_ctx_t *network_ctx = (network_ctx_t *)pvParams;

    if (network_ctx == NULL) {
        goto cleanup;
    }

    static uint8_t tx_buffer[TX_BUFFER_LEN] = {0};
    static server_payload_t payload = {0};

    uint32_t server_connected;
    xTaskNotify(
        xTaskGetCurrentTaskHandle(), SERVER_DISCONNECTED, eSetValueWithOverwrite
    );

    while (1) {
        // only pause when server is disconnected
        xTaskNotifyWait(
            0,
            0,
            &server_connected,
            (server_connected == SERVER_CONNECTED) ? 0 : portMAX_DELAY
        );

        if (server_connected == SERVER_CONNECTED) {
            // timeout after 100ms to check if server is still alive
            if (xQueueReceive(
                    network_ctx->tcp_send_queue_handle,
                    &payload,
                    pdMS_TO_TICKS(100)
                ) == pdFALSE) {
                continue;
            }

            size_t packet_len = sizeof(tx_buffer);

            // convert packet struct to bytes depending on type
            switch (payload.packet_type) {
            case PT_CONFIG:
                make_config_packet(
                    tx_buffer, &packet_len, &payload.payload_data.config
                );
                break;
            case PT_DATA:
                make_data_packet(
                    tx_buffer, &packet_len, &payload.payload_data.data
                );
                break;
            case PT_STATUS:
                make_status_packet(
                    tx_buffer, &packet_len, &payload.payload_data.status
                );
                break;
            case PT_ACK:
                make_ack_packet(
                    tx_buffer, &packet_len, &payload.payload_data.ack
                );
                break;
            case PT_NACK:
                make_nack_packet(
                    tx_buffer, &packet_len, &payload.payload_data.nack
                );
                break;
            default:
                ESP_LOGE(TAG, "Attempted to send invalid server packet");
                continue;
            }

            int32_t len_sent = send(
                network_ctx->server_sock, tx_buffer, packet_len, 0
            );
            if (len_sent < 0) {
                ESP_LOGE(TAG, "send failed: errno %d", errno);
                switch (errno) {
                case ECONNRESET:
                case EBADF:
                    continue;
                default:
                    goto cleanup;
                }
            }
            ESP_LOGI(TAG, "Outgoing packet len: %d", len_sent);
        }
    }

cleanup:
    // delete task here
    ;
}