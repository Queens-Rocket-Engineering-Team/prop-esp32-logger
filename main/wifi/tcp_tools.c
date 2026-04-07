#include <esp_err.h>
#include <esp_log.h>
#include <netdb.h>
#include <string.h>
#include <sys/socket.h>

#include "config_json.h"
#include "qret_protocol.h"
#include "setup.h"
#include "wifi_tools.h"

#define RX_BUFFER_LEN 2048
#define TX_BUFFER_LEN 8192

static const char *TAG = "TCP";

esp_err_t tcp_connect_to_server(int32_t *sock, const char server_ip[], uint16_t server_port) {
    if (sock == NULL || server_ip == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    char server_port_str[10] = {0};
    snprintf(server_port_str, sizeof(server_port_str), "%u", server_port);

    esp_err_t ret = ESP_FAIL;
    *sock = -1;

    int32_t err;

    struct addrinfo hints = {0}, *res = NULL;
    // set up TCP parameters for socket
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    err = getaddrinfo(server_ip, server_port_str, &hints, &res);
    if (err != 0) {
        ret = ESP_FAIL;
        goto cleanup;
    }
    // creating client's socket
    *sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (*sock < 0) {
        ret = ESP_FAIL;
        goto cleanup;
    }

    int enable = 1;
    err = setsockopt(*sock, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable));
    if (err != 0) {
        ret = ESP_FAIL;
        goto cleanup;
    }

    // attempt to connect to the server
    err = connect(*sock, res->ai_addr, res->ai_addrlen);
    if (err != 0) {
        ESP_LOGE(TAG, "connect failed: errno %d", errno);
        goto cleanup;
    }
    ESP_LOGI(TAG, "Connected to server");
    ret = ESP_OK;

cleanup:
    if (res != NULL) {
        freeaddrinfo(res);
    }
    if (*sock != -1 && ret != ESP_OK) {
        close(*sock);
        *sock = -1;
    }
    return ret;
}

void tcp_client_recv(void *pvParams) {
    network_ctx_t *network_ctx = (network_ctx_t *)pvParams;

    static uint8_t rx_buffer[RX_BUFFER_LEN] = {0};
    static uint16_t packet_len = 0;
    static client_payload_t payload = {0};

    qret_protocol_ret_t ret = PROTOCOL_OK;

    while (1) {
        // only pause when server is disconnected
        xEventGroupWaitBits(network_ctx->wifi_event_group_handle, WIFI_CONNECTED_BIT, pdFALSE, pdFALSE, portMAX_DELAY);

        // get the full header from a packet
        int32_t header_len_recv = recv(network_ctx->server_tcp_sock, rx_buffer, HEADER_SIZE, MSG_WAITALL);
        if (header_len_recv < 0) {
            ESP_LOGE(TAG, "recv failed: errno %d", errno);
            continue;
        } else if (header_len_recv == 0) {
            ESP_LOGI(TAG, "Connection closed by server gracefully");
            continue;
        }

        get_packet_len(rx_buffer, HEADER_SIZE, &packet_len);
        int32_t data_len_recv = 0;

        if (packet_len - HEADER_SIZE > 0) {
            if (packet_len > sizeof(rx_buffer) - HEADER_SIZE) {
                ESP_LOGE(TAG, "rx_buffer ran out of space: packet len %d", packet_len);
                break;
            }
            // recieve the rest of the packet if not header only
            data_len_recv = recv(network_ctx->server_tcp_sock, rx_buffer + HEADER_SIZE, packet_len, MSG_WAITALL);
            if (data_len_recv < 0) {
                ESP_LOGE(TAG, "recv failed: errno %d", errno);
                continue;
            } else if (data_len_recv == 0) {
                ESP_LOGI(TAG, "Connection closed by server gracefully");
                continue;
            }
        }

        ESP_LOGD(TAG, "Received %d bytes from server:", header_len_recv + data_len_recv);
        ESP_LOG_BUFFER_HEXDUMP(TAG, rx_buffer, header_len_recv + data_len_recv, ESP_LOG_DEBUG);

        // parse and send packet data for processing
        ret = client_parse_packet(rx_buffer, sizeof(rx_buffer), &payload);
        if (ret != PROTOCOL_OK) {
            ESP_LOGE(TAG, "QRET protocol err:", ret);
        }
        xQueueSend(network_ctx->tcp_recv_queue_handle, (void *)&payload, 0);
    }
}

void tcp_client_send(void *pvParams) {
    network_ctx_t *network_ctx = (network_ctx_t *)pvParams;

    static uint8_t tx_buffer[TX_BUFFER_LEN] = {0};
    static server_payload_t payload = {0};

    qret_protocol_ret_t ret = PROTOCOL_OK;

    while (1) {
        // only pause when server is disconnected
        xEventGroupWaitBits(network_ctx->wifi_event_group_handle, WIFI_CONNECTED_BIT, pdFALSE, pdFALSE, portMAX_DELAY);

        // timeout after 100ms to check if server is still alive
        if (xQueueReceive(network_ctx->tcp_send_queue_handle, &payload, pdMS_TO_TICKS(100)) == pdFALSE) {
            continue;
        }

        size_t packet_len = sizeof(tx_buffer);

        // convert packet struct to bytes depending on type
        switch (payload.packet_type) {
        case PT_CONFIG:
            ret = make_config_packet(tx_buffer, &packet_len, &payload.payload_data.config);
            break;
        case PT_DATA:
            make_data_packet(tx_buffer, &packet_len, &payload.payload_data.data);
            break;
        case PT_STATUS:
            make_status_packet(tx_buffer, &packet_len, &payload.payload_data.status);
            break;
        case PT_ACK:
            make_ack_packet(tx_buffer, &packet_len, &payload.payload_data.ack);
            break;
        case PT_NACK:
            make_nack_packet(tx_buffer, &packet_len, &payload.payload_data.nack);
            break;
        default:
            ESP_LOGE(TAG, "Attempted to send invalid server packet");
            continue;
        }
        if (ret != PROTOCOL_OK) {
            ESP_LOGE(TAG, "QRET protocol err:", ret);
        }

        int32_t len_sent = send(network_ctx->server_tcp_sock, tx_buffer, packet_len, 0);
        if (len_sent < 0) {
            ESP_LOGE(TAG, "send failed: errno %d", errno);
            continue;
        }
        ESP_LOGD(TAG, "Outgoing packet len: %d", len_sent);
        ESP_LOG_BUFFER_HEXDUMP(TAG, tx_buffer, len_sent, ESP_LOG_DEBUG);
    }
}