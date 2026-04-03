#ifndef WIFI_TOOLS_H
#define WIFI_TOOLS_H

#include <esp_err.h>
#include <esp_netif.h>
#include <freertos/FreeRTOS.h>
#include <stdint.h>

typedef struct {
    esp_netif_t *netif_handle;
    uint32_t server_ip;
    uint16_t server_port;
    int32_t server_sock;
    TaskHandle_t network_manager_handle;
    TaskHandle_t ssdp_handle;
    TaskHandle_t tcp_conn_handle;
} network_ctx_t;

enum {
    SIG_WIFI_DISCONN = (1 << 0),
    SIG_WIFI_CONN = (1 << 1),
    SIG_SSDP_GOT_SERVER = (1 << 2),
    SIG_TCP_CONN_SERVER = (1 << 3),
    SIG_TCP_RECV = (1 << 4),
};

esp_err_t wifi_event_handler_register(network_ctx_t *network_ctx);

void network_state_manager(void *pvParams);

void ssdp_discover_server(void *pvParams);

void tcp_connect_to_server(void *pvParams);

// void tcp_client_recv_task(void *pvParams);

#endif