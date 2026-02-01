#ifndef CONTROL_H
#define CONTROL_H

#include <driver/gpio.h>
#include <esp_err.h>
#include <stdint.h>

// Control devices such as relays, solenoids etc.

typedef enum { CONTROL_OPEN, CONTROL_CLOSED, CONTROL_UNKNOWN } control_state_t;

typedef enum {
    OPEN_LOW, // Actuator open when GPIO low
    OPEN_HIGH // Actuator open when GPIO high
} control_active_t;

typedef struct {
    gpio_num_t gpio_num;
    control_state_t state;
    control_active_t active;
} control_t;

esp_err_t set_control(control_t *control, control_state_t state);

// Initialize control pin
esp_err_t control_init(control_t *control,
                       gpio_num_t gpio_num,
                       control_state_t state,
                       control_active_t active);

esp_err_t open_control(control_t *control);

esp_err_t close_control(control_t *control);

control_state_t get_control_state(const control_t *control);

#endif