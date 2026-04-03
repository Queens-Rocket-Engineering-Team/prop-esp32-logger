#include "wifi_tools.h"
#include <esp_err.h>
#include <esp_event.h>
#include <esp_log.h>
#include <esp_netif.h>
#include <esp_wifi.h>
#include <netdb.h>
#include <sys/socket.h>

#define SSDP_STACK_SIZE 4096
#define TCP_CONN_STACK_SIZE 4096

static const char TAG[] = "NETWORK MANAGER";

void network_state_manager(void *pvParams) {
    if (pvParams == NULL) {
        abort();
    }

    network_ctx_t *network_ctx = (network_ctx_t *)pvParams;
    uint32_t signal = SIG_WIFI_DISCONN;

    static StaticTask_t xTaskBufferSSDP;
    static StackType_t xStackSSDP[SSDP_STACK_SIZE];

    static StaticTask_t xTaskBufferTCPCONN;
    static StackType_t xStackTCPCONN[TCP_CONN_STACK_SIZE];

    while (1) {
        xTaskNotifyWait(0, UINT32_MAX, &signal, portMAX_DELAY);

        if (signal & SIG_WIFI_DISCONN) {
            // reset state manager if wifi disconnects
            signal = SIG_WIFI_DISCONN;
            if (network_ctx->ssdp_handle != NULL) {
                vTaskDelete(network_ctx->ssdp_handle);
                network_ctx->ssdp_handle = NULL;
            }
            if (network_ctx->tcp_conn_handle != NULL) {
                vTaskDelete(network_ctx->tcp_conn_handle);
                network_ctx->tcp_conn_handle = NULL;
            }
        }
        if ((signal & SIG_WIFI_CONN) && network_ctx->ssdp_handle == NULL) {
            network_ctx->ssdp_handle = xTaskCreateStatic(
                ssdp_discover_server,
                "SSDP",
                SSDP_STACK_SIZE,
                (void *)network_ctx,
                1,
                xStackSSDP,
                &xTaskBufferSSDP
            );
        }
        if ((signal & SIG_SSDP_GOT_SERVER) && network_ctx->tcp_conn_handle == NULL) {
            network_ctx->tcp_conn_handle = xTaskCreateStatic(
                tcp_connect_to_server,
                "TCP conn",
                TCP_CONN_STACK_SIZE,
                (void *)network_ctx,
                1,
                xStackTCPCONN,
                &xTaskBufferTCPCONN
            );

            // debug
            char test[IPADDR_STRLEN_MAX];
            inet_ntop(AF_INET, &network_ctx->server_ip, test, sizeof test);
            printf("%s\n", test);
            // debug
        }
        if (signal & SIG_TCP_CONN_SERVER) {
            // xTaskNotify(); for main recieve task
            ;
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
            ESP_LOGD(TAG, "Disconnected from AP");
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
