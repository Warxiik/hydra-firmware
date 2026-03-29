#include "gpio.h"
#include "pins.h"
#include "protocol_defs.h"
#include "hardware/gpio.h"
#include "hardware/pwm.h"

static const uint8_t heater_pins[] = {
    PIN_HEATER_MANIFOLD, PIN_HEATER_BED
};

static const uint8_t fan_pins[] = {
    PIN_FAN_PART, PIN_FAN_HE
};

static const uint8_t endstop_pins[] = {
    PIN_ENDSTOP_X, PIN_ENDSTOP_Y, PIN_ENDSTOP_Z
};

void hydra_gpio_init(void) {
    /* Heater outputs (start OFF) */
    for (int i = 0; i < 2; i++) {
        gpio_init(heater_pins[i]);
        gpio_set_dir(heater_pins[i], GPIO_OUT);
        gpio_put(heater_pins[i], 0);
    }

    /* Fan PWM outputs */
    for (int i = 0; i < 2; i++) {
        gpio_set_function(fan_pins[i], GPIO_FUNC_PWM);
        uint slice = pwm_gpio_to_slice_num(fan_pins[i]);
        pwm_set_wrap(slice, 4999); /* 125MHz / 5000 = 25kHz */
        pwm_set_chan_level(slice, pwm_gpio_to_channel(fan_pins[i]), 0);
        pwm_set_enabled(slice, true);
    }

    /* Endstop inputs with pull-ups */
    for (int i = 0; i < 3; i++) {
        gpio_init(endstop_pins[i]);
        gpio_set_dir(endstop_pins[i], GPIO_IN);
        gpio_pull_up(endstop_pins[i]);
    }

    /* Status LED */
    gpio_init(PIN_LED_STATUS);
    gpio_set_dir(PIN_LED_STATUS, GPIO_OUT);
    gpio_put(PIN_LED_STATUS, 1);
}

void hydra_gpio_set_pwm(uint8_t channel, uint16_t duty) {
    if (channel < 2) {
        /* Heater channels: software PWM (just on/off for now) */
        gpio_put(heater_pins[channel], duty > 0 ? 1 : 0);
    } else if (channel < PWM_CHANNEL_COUNT) {
        /* Fan channels: hardware PWM */
        uint8_t fan_idx = channel - 2;
        uint slice = pwm_gpio_to_slice_num(fan_pins[fan_idx]);
        uint chan = pwm_gpio_to_channel(fan_pins[fan_idx]);
        /* Scale 0-65535 to 0-4999 */
        uint16_t level = (uint32_t)duty * 4999 / 65535;
        pwm_set_chan_level(slice, chan, level);
    }
}

void hydra_gpio_kill_heaters(void) {
    for (int i = 0; i < 2; i++) {
        gpio_put(heater_pins[i], 0);
    }
}

uint8_t hydra_gpio_read_endstops(void) {
    uint8_t state = 0;
    for (int i = 0; i < 3; i++) {
        if (!gpio_get(endstop_pins[i])) { /* Active low with pull-up */
            state |= (1 << i);
        }
    }
    return state;
}
