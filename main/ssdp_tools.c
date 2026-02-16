#include "ssdp_tools.h"
#include <esp_err.h>
#include <esp_log.h>
#include <esp_netif.h>
#include <getnameinfo.h>
#include <netdb.h>
#include <stddef.h>
#include <string.h>
#include <sys/socket.h>

#define SSDP_PORT "1900"
#define SSDP_IP "239.255.255.250"
#define SSDP_ANY_IP "0.0.0.0"

static const char *TAG = "SSDP";

esp_err_t ssdp_discover_server(char server_ip[],
                               size_t server_ip_len,
                               esp_netif_t *netif_handle) {

    if (server_ip_len < IPADDR_STRLEN_MAX) {
        return ESP_ERR_NO_MEM;
    }

    int err;

    // Creating client's socket

    struct addrinfo hints, *res; // Set up UDP parameters for socket
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_protocol = IPPROTO_UDP;

    err = getaddrinfo(SSDP_ANY_IP, SSDP_PORT, &hints, &res);
    if (err != 0) {
        return ESP_FAIL;
    }

    int sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sock == -1) {
        freeaddrinfo(res);
        return ESP_FAIL;
    }

    int enable = 1;
    err = setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof enable);
    if (err != 0) {
        close(sock);
        freeaddrinfo(res);
        return ESP_FAIL;
    }

    // Bind the socket to port 1900, any ip
    err = bind(sock, res->ai_addr, res->ai_addrlen);
    if (err != 0) {
        close(sock);
        freeaddrinfo(res);
        return ESP_FAIL;
    }
    freeaddrinfo(res);

    // Add membership to SSDP multicast ip
    esp_netif_ip_info_t ip_info = {0};
    esp_netif_get_ip_info(netif_handle, &ip_info);

    struct in_addr local_addr;
    inet_addr_from_ip4addr(&local_addr, &ip_info.ip);

    ip_mreq imreq = {0};
    imreq.imr_interface.s_addr = local_addr.s_addr;
    err = inet_pton(AF_INET, SSDP_IP, &imreq.imr_multiaddr.s_addr);
    if (err != 1) {
        close(sock);
        return ESP_FAIL;
    }
    err = setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, &imreq, sizeof imreq);
    if (err != 0) {
        close(sock);
        return ESP_FAIL;
    }

    // Listen for SSDP M-SEARCH from server
    struct sockaddr_in remote_addr;
    socklen_t remote_addr_len;
    char buf[1024];

    while (1) {
        remote_addr_len = sizeof remote_addr;
        ssize_t len =
            recvfrom(sock, buf, sizeof buf, 0, (struct sockaddr *)&remote_addr,
                     &remote_addr_len);

        if (len < 0) {
            close(sock);
            return ESP_FAIL;
        }

        buf[len] = '\0'; // recvfrom does not null terminate buffer

        ESP_LOGD(TAG, "%s", buf);

        if (strcasestr(buf, "M-SEARCH * HTTP/1.1") != NULL &&
            strcasestr(buf, "HOST: 239.255.255.250:1900") != NULL &&
            strcasestr(buf, "ST: urn:qretprop:espdevice:1") != NULL) {
            break;
        }
    }

    close(sock);

    char port_str[6];
    getnameinfo((struct sockaddr *)&remote_addr, remote_addr_len, server_ip,
                server_ip_len, port_str, sizeof port_str, 0);

    return ESP_OK;
}