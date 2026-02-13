#ifndef SSDP_TOOLS_H
#define SSDP_TOOLS_H

#include <esp_netif.h>
#include <stddef.h>

void ssdp_discover_server(char server_ip[],
                          size_t ip_server_len,
                          esp_netif_t *netif_handle);

#endif