#include "wifi_tools.h"
#include "esp_config_json.h"
#include "setup.h"
#include <esp_err.h>
#include <string.h>
#include <sys/socket.h>

esp_err_t tcp_connect_to_server(char server_ip[],
                                uint16_t server_port,
                                int *sock) {

    int err;

    struct sockaddr_in dest_addr = {0};
    err = inet_pton(AF_INET, server_ip, &dest_addr.sin_addr);
    if (err != 1) {
        return ESP_FAIL;
    }
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(server_port);

    // Creating client's socket
    *sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    int enable = 1;
    err = setsockopt(*sock, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable));
    if (err != 0) {
        close(*sock);
        return ESP_FAIL;
    }
    err = connect(*sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
    if (err != 0) {
        close(*sock);
        return ESP_FAIL;
    }
    return ESP_OK;
}

void tcp_client_recv_task(void *pvParams) {

    app_data_t *app_data = (app_data_t *)pvParams;
    char rx_buffer[128];

    while (true) {

        int len = recv(app_data->sock, rx_buffer, sizeof(rx_buffer) - 1, 0);
        // Error occurred during receiving
        if (len < 0) {
            // ESP_LOGE(TAG, "recv failed: errno %d", errno);
            break;
        }

        // Data received
        else {
            rx_buffer[len] = 0; // Null-terminate whatever we received and treat
                                // like a string
            // ESP_LOGI(TAG, "Received %d bytes from %s:", len, host_ip);
            // ESP_LOGI(TAG, "%s", rx_buffer);
        }
    }

    if (app_data->sock != -1) {
        // ESP_LOGE(TAG, "Shutting down socket and restarting...");
        shutdown(app_data->sock, 0);
        close(app_data->sock);
        // delete task here
    }
}

void send_json(int sock) {



    send(sock, json_config, strlen(json_config), 0);


}