#include "esp_config_json.h"
#include "wifi_tools.h"
#include <esp_err.h>
#include <esp_event.h>
#include <esp_log.h>
#include <esp_netif.h>
#include <esp_wifi.h>
#include <netdb.h>
#include <sys/socket.h>

#define TCP_RECV_STACK_SIZE 8192
#define TCP_SEND_STACK_SIZE 16384

static const char *TAG = "Network Manager";

void network_state_manager(void *pvParams) {
    if (pvParams == NULL) {
        abort();
    }

    network_ctx_t *network_ctx = (network_ctx_t *)pvParams;
    uint32_t signal = SIG_WIFI_DISCONN;

    esp_err_t err;

    // create the recv/send tasks on network startup

    static StaticTask_t xTaskBufferRECV;
    static StackType_t xStackRECV[TCP_RECV_STACK_SIZE];

    network_ctx->tcp_recv_handle = xTaskCreateStatic(
        tcp_client_recv,
        "Server RECV",
        TCP_RECV_STACK_SIZE,
        (void *)network_ctx,
        3,
        xStackRECV,
        &xTaskBufferRECV
    );

    static StaticTask_t xTaskBufferSEND;
    static StackType_t xStackSEND[TCP_SEND_STACK_SIZE];

    network_ctx->tcp_send_handle = xTaskCreateStatic(
        tcp_client_send,
        "Server SEND",
        TCP_SEND_STACK_SIZE,
        (void *)network_ctx,
        3,
        xStackSEND,
        &xTaskBufferSEND
    );

    while (1) {
        xTaskNotifyWait(0, UINT32_MAX, &signal, portMAX_DELAY);

        if (signal & SIG_WIFI_DISCONN) {
            // reset state manager if wifi disconnects
            signal = SIG_WIFI_DISCONN;
            xTaskNotify(
                network_ctx->tcp_recv_handle,
                SERVER_DISCONNECTED,
                eSetValueWithOverwrite
            );
            xTaskNotify(
                network_ctx->tcp_send_handle,
                SERVER_DISCONNECTED,
                eSetValueWithOverwrite
            );
            if (network_ctx->ssdp_sock != -1) {
                close(network_ctx->ssdp_sock);
                network_ctx->ssdp_sock = -1;
            }
            if (network_ctx->server_sock != -1) {
                close(network_ctx->server_sock);
                network_ctx->server_sock = -1;
            }
        }
        if (signal & SIG_WIFI_CONN) {
            // look for server when wifi connects
            err = ssdp_discover_server(
                &network_ctx->ssdp_sock,
                &network_ctx->server_ip,
                network_ctx->netif_handle
            );
            if (err == ESP_OK) {
                xTaskNotify(
                    xTaskGetCurrentTaskHandle(), SIG_SSDP_GOT_SERVER, eSetBits
                );
            }
        }
        if (signal & SIG_SSDP_GOT_SERVER) {
            // attempt to connect to server when ip found
            err = tcp_connect_to_server(
                &network_ctx->server_sock,
                network_ctx->server_ip,
                network_ctx->server_port
            );
            if (err == ESP_OK) {
                xTaskNotify(
                    xTaskGetCurrentTaskHandle(), SIG_TCP_CONN_SERVER, eSetBits
                );
            }
        }
        if (signal & SIG_TCP_CONN_SERVER) {
            // enable the recv/send tasks when connected to server
            xTaskNotify(
                network_ctx->tcp_recv_handle,
                SERVER_CONNECTED,
                eSetValueWithOverwrite
            );
            xTaskNotify(
                network_ctx->tcp_send_handle,
                SERVER_CONNECTED,
                eSetValueWithOverwrite
            );

            // TODO take this out later, should be in main but lazy
            server_payload_t payload = {
                .packet_type = PT_CONFIG,
                .payload_data = {
                    .config = {
                        .json_config = json_config_str,
                        .json_config_len = sizeof(json_config_str),
                        0,
                        0
                    }
                }
            };
            xQueueSend(network_ctx->tcp_send_queue_handle, (void *)&payload, 0);
        }
    }
}

static void wifi_event_handler(
    void *arg,
    esp_event_base_t event_base,
    int32_t event_id,
    void *event_data
) {

    TaskHandle_t *network_manager_task_handle = (TaskHandle_t *)arg;

    if (event_base == WIFI_EVENT) {
        switch (event_id) {
        case WIFI_EVENT_STA_START:
            esp_wifi_connect();
            break;
        case WIFI_EVENT_STA_DISCONNECTED:
            ESP_LOGI(TAG, "Disconnected from AP");
            esp_wifi_connect(); // TODO add retry counter
            break;
        case WIFI_EVENT_STA_CONNECTED:
            ESP_LOGI(TAG, "Connected to AP");
            break;
        }

    } else if (event_base == IP_EVENT) {
        switch (event_id) {
        case IP_EVENT_STA_GOT_IP:
            ESP_LOGI(TAG, "Got IP");
            if (network_manager_task_handle != NULL) {
                xTaskNotify(
                    *network_manager_task_handle, SIG_WIFI_CONN, eSetBits
                );
            }
            break;
        case IP_EVENT_STA_LOST_IP:
            ESP_LOGI(TAG, "Lost IP");
            if (network_manager_task_handle != NULL) {
                xTaskNotify(
                    *network_manager_task_handle, SIG_WIFI_DISCONN, eSetBits
                );
            }
            break;
        }
    }
}

esp_err_t wifi_event_handler_register(network_ctx_t *network_ctx) {

    esp_err_t ret;
    ret = esp_event_handler_instance_register(
        WIFI_EVENT,
        ESP_EVENT_ANY_ID,
        &wifi_event_handler,
        &network_ctx->network_manager_handle,
        NULL
    );

    if (ret != ESP_OK) {
        return ret;
    }

    ret = esp_event_handler_instance_register(
        IP_EVENT,
        IP_EVENT_STA_GOT_IP,
        &wifi_event_handler,
        &network_ctx->network_manager_handle,
        NULL
    );

    return ret;
}
