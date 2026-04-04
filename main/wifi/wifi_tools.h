#ifndef WIFI_TOOLS_H
#define WIFI_TOOLS_H

#include "qret_protocol.h"
#include <esp_err.h>
#include <esp_netif.h>
#include <freertos/FreeRTOS.h>
#include <stdint.h>

typedef struct {
    esp_netif_t *netif_handle;
    uint32_t server_ip;
    uint16_t server_port;
    int32_t server_sock;
    int32_t ssdp_sock;
    QueueHandle_t tcp_recv_queue_handle;
    QueueHandle_t tcp_send_queue_handle;
    TaskHandle_t network_manager_handle;
    TaskHandle_t tcp_recv_handle;
    TaskHandle_t tcp_send_handle;
} network_ctx_t;

enum {
    SIG_WIFI_DISCONN = (1 << 0),
    SIG_WIFI_CONN = (1 << 1),
    SIG_SSDP_GOT_SERVER = (1 << 2),
    SIG_TCP_CONN_SERVER = (1 << 3),
    SIG_TCP_RECV = (1 << 4),
};

enum {
    SERVER_CONNECTED = 0,
    SERVER_DISCONNECTED
};

#define TCP_SERVER_PORT 50000
#define TCP_QUEUE_LEN 10
#define TCP_QUEUE_ITEM_SIZE sizeof(server_payload_t)

esp_err_t wifi_event_handler_register(network_ctx_t *network_ctx);

void network_state_manager(void *pvParams);

esp_err_t ssdp_discover_server(
    int32_t *sock,
    uint32_t *server_ip,
    esp_netif_t *netif_handle
);

esp_err_t tcp_connect_to_server(
    int32_t *sock,
    uint32_t server_ip,
    uint16_t server_port
);

void tcp_client_recv(void *pvParams);

void tcp_client_send(void *pvParams);

#endif