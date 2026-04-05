#ifndef SENSOR_H
#define SENSOR_H

#include "ads112c04.h"
#include <stdint.h>

typedef struct {
    ads112c04_t ADC;
    int8_t high_pin;
    int8_t low_pin;
} sensor_t;

sensor_voltage_reading();

#endif