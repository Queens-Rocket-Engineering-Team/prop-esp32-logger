#ifndef TCP_TOOLS_H
#define TCP_TOOLS_H

#include <esp_err.h>
#include <stdint.h>

esp_err_t tcp_connect_to_server(char server_ip[],
                                uint16_t server_port,
                                int *sock);

void tcp_client_recv_task(void *pvParams);

#endif