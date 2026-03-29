#include "valve_control.h"
#include "pins.h"
#include "protocol_defs.h"
#include "hardware/gpio.h"

static const uint8_t valve_pins[VALVE_COUNT] = {
    PIN_VALVE_0, PIN_VALVE_1, PIN_VALVE_2, PIN_VALVE_3
};

static uint8_t current_mask = 0;

void valve_control_init(void) {
    for (int i = 0; i < VALVE_COUNT; i++) {
        gpio_init(valve_pins[i]);
        gpio_set_dir(valve_pins[i], GPIO_OUT);
        gpio_put(valve_pins[i], 0);  /* All valves closed at startup */
    }
    current_mask = 0;
}

void valve_control_set(uint8_t mask) {
    mask &= VALVE_ALL;
    for (int i = 0; i < VALVE_COUNT; i++) {
        gpio_put(valve_pins[i], (mask >> i) & 1);
    }
    current_mask = mask;
}

uint8_t valve_control_get(void) {
    return current_mask;
}

void valve_control_close_all(void) {
    valve_control_set(0);
}
