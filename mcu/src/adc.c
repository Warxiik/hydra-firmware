#include "adc.h"
#include "pins.h"
#include "config.h"
#include "hardware/adc.h"

/* ADC channel mapping: GP27=ADC1 (manifold), GP28=ADC2 (bed) */
static const uint8_t adc_channels[] = {1, 2}; /* Manifold, Bed */
static uint16_t adc_values[ADC_CHANNEL_COUNT];
static uint8_t current_channel;

void hydra_adc_init(void) {
    adc_init();
    adc_gpio_init(PIN_ADC_THERM_MANIFOLD);
    adc_gpio_init(PIN_ADC_THERM_BED);
    current_channel = 0;
}

void hydra_adc_poll(void) {
    /* Round-robin: read one channel per call */
    adc_select_input(adc_channels[current_channel]);
    adc_values[current_channel] = adc_read();
    current_channel = (current_channel + 1) % ADC_CHANNEL_COUNT;
}

void hydra_adc_read_all(uint16_t out[ADC_CHANNEL_COUNT]) {
    for (int i = 0; i < ADC_CHANNEL_COUNT; i++) {
        out[i] = adc_values[i];
    }
}
