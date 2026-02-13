#include "tcp_tools.h"
#include <sys/socket.h>
#include <string.h>
#include "setup.h"

void tcp_connect_to_server(const char server_ip[], uint16_t server_port, int *sock) { // TODO add error handling

    struct sockaddr_in dest_addr = {0};
    inet_pton(AF_INET, server_ip, &dest_addr.sin_addr);
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(server_port);

    // Creating client's socket
    *sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    int enable = true;
    setsockopt(*sock, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable));
    connect(*sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));

}

void tcp_client_recv_task(void *pvParams) {

    app_data_t *app_data = (app_data_t *) pvParams;
    char rx_buffer[128];

    while (true) {


            int len = recv(app_data->sock, rx_buffer, sizeof(rx_buffer) - 1, 0);
            // Error occurred during receiving
            if (len < 0) {
                //ESP_LOGE(TAG, "recv failed: errno %d", errno);
                break;
            }




            // Data received
            else {
                rx_buffer[len] = 0; // Null-terminate whatever we received and treat like a string
                //ESP_LOGI(TAG, "Received %d bytes from %s:", len, host_ip);
                //ESP_LOGI(TAG, "%s", rx_buffer);
            }
        }

        if (app_data->sock != -1) {
            //ESP_LOGE(TAG, "Shutting down socket and restarting...");
            shutdown(app_data->sock, 0);
            close(app_data->sock);
            // delete task here
        }
    }



