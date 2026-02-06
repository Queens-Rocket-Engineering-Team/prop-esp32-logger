#include "control.h"
#include <stdbool.h>

esp_err_t control_init(control_t *control,
                       gpio_num_t gpio_num,
                       control_state_t state,
                       control_active_t active) {

    if (!control) {
        return ESP_ERR_INVALID_ARG;
    }
    if (!GPIO_IS_VALID_OUTPUT_GPIO(gpio_num)) {
        return ESP_ERR_INVALID_ARG;
    }
    if (state != CONTROL_OPEN && state != CONTROL_CLOSED) {
        return ESP_ERR_INVALID_ARG;
    }
    if (active != OPEN_LOW && active != OPEN_HIGH) {
        return ESP_ERR_INVALID_ARG;
    }

    control->gpio_num = gpio_num;
    control->state = CONTROL_UNKNOWN; // State will be if set_control succeeds
    control->active = active;

    esp_err_t ret;
    ret = gpio_reset_pin(gpio_num);
    if (ret != ESP_OK) {
        return ret;
    }

    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << control->gpio_num),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,     // Disable pull-up
        .pull_down_en = GPIO_PULLDOWN_DISABLE, // Disable pull-down
        .intr_type = GPIO_INTR_DISABLE         // Disable interrupts
    };
    ret = gpio_config(&io_conf);
    if (ret != ESP_OK) {
        return ret;
    }
    return set_control(control, state);
}

esp_err_t open_control(control_t *control) {
    return set_control(control, CONTROL_OPEN);
}

esp_err_t close_control(control_t *control) {
    return set_control(control, CONTROL_CLOSED);
}

esp_err_t set_control(control_t *control, control_state_t state) {
    if (!control) {
        return ESP_ERR_INVALID_ARG;
    }
    if (state == CONTROL_UNKNOWN) {
        return ESP_ERR_INVALID_ARG;
    }

    bool level;
    if (control->active == OPEN_LOW) { // Choose correct level for desired state
        level = (state == CONTROL_CLOSED);
    } else if (control->active == OPEN_HIGH) {
        level = (state == CONTROL_OPEN);
    } else {
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t ret;
    ret = gpio_set_level(control->gpio_num, level);
    if (ret != ESP_OK) {
        return ret;
    }
    control->state = state;
    return ESP_OK;
}

control_state_t get_control_state(const control_t *control) {
    if (!control) {
        return CONTROL_UNKNOWN;
    }
    return control->state;
}