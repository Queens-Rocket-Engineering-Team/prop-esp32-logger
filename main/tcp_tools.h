#ifndef TCP_TOOLS_H
#define TCP_TOOLS_H

#include <stdint.h>

void tcp_connect_to_server(const char server_ip[], uint16_t server_port, int *sock);

void tcp_client_recv_task(void *pvParams);

#endif