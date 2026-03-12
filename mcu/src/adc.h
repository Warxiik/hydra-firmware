#ifndef HYDRA_MCU_ADC_H
#define HYDRA_MCU_ADC_H

#include <stdint.h>
#include "protocol_defs.h"

void hydra_adc_init(void);
void hydra_adc_poll(void);
void hydra_adc_read_all(uint16_t out[ADC_CHANNEL_COUNT]);

#endif /* HYDRA_MCU_ADC_H */
