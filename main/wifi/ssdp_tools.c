#include "wifi_tools.h"
#include <esp_err.h>
#include <esp_log.h>
#include <esp_netif.h>
#include <getnameinfo.h>
#include <netdb.h>
#include <stdint.h>
#include <string.h>

#define SSDP_PORT "1900"
#define SSDP_IP "239.255.255.250"
#define SSDP_ANY_IP "0.0.0.0"

static const char *TAG = "SSDP";

esp_err_t ssdp_discover_server(
    int32_t *sock,
    uint32_t *server_ip,
    esp_netif_t *netif_handle
) {

    if (sock == NULL || netif_handle == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t ret = ESP_FAIL;
    *sock = -1;

    // Creating client's socket
    int32_t err;
    struct addrinfo hints = {0},
                    *res = NULL; // Set up UDP parameters for socket

    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_protocol = IPPROTO_UDP;

    err = getaddrinfo(SSDP_ANY_IP, SSDP_PORT, &hints, &res);
    if (err != 0) {
        ret = ESP_FAIL;
        goto cleanup;
    }

    *sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (*sock < 0) {
        freeaddrinfo(res);
        ret = ESP_FAIL;
        goto cleanup;
    }

    int enable = 1;
    err = setsockopt(*sock, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof enable);
    if (err != 0) {
        freeaddrinfo(res);
        ret = ESP_FAIL;
        goto cleanup;
    }

    // Bind the socket to port 1900, any ip
    err = bind(*sock, res->ai_addr, res->ai_addrlen);
    if (err != 0) {
        freeaddrinfo(res);
        ret = ESP_FAIL;
        goto cleanup;
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
        ret = ESP_FAIL;
        goto cleanup;
    }
    err = setsockopt(
        *sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, &imreq, sizeof imreq
    );
    if (err != 0) {
        ret = ESP_FAIL;
        goto cleanup;
    }

    // Listen for SSDP M-SEARCH from server
    struct sockaddr_in remote_addr;
    socklen_t remote_addr_len;
    char buf[1024];

    while (1) {
        remote_addr_len = sizeof remote_addr;
        ssize_t len = recvfrom(
            *sock,
            buf,
            sizeof buf - 1,
            0,
            (struct sockaddr *)&remote_addr,
            &remote_addr_len
        );

        if (len < 0) {
            ret = ESP_FAIL;
            goto cleanup;
        }

        buf[len] = '\0'; // recvfrom does not null terminate buffer

        ESP_LOGD(TAG, "%s", buf);

        if (strcasestr(buf, "M-SEARCH * HTTP/1.1") != NULL &&
            strcasestr(buf, "HOST: 239.255.255.250:1900") != NULL &&
            strcasestr(buf, "ST: urn:qretprop:espdevice:1") != NULL) {
            break;
        }
    }

    *server_ip = remote_addr.sin_addr.s_addr;

    ret = ESP_OK;

cleanup:
    if (*sock != -1) {
        close(*sock);
    }
    *sock = -1;

    return ret;
}