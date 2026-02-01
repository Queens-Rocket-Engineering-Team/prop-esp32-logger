#include <esp_err.h>
#include "control.h"

esp_err_t setup();
void run();

void app_main(void) {
    setup();

    while(true) {
        run();
    }
}

esp_err_t setup() {
    esp_err_t status = ESP_OK;
    return status;
}

void run() {

}