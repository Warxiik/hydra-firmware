#ifndef HYDRA_MCU_GPIO_H
#define HYDRA_MCU_GPIO_H

#include <stdint.h>

void hydra_gpio_init(void);
void hydra_gpio_set_pwm(uint8_t channel, uint16_t duty);
void hydra_gpio_kill_heaters(void);
uint8_t hydra_gpio_read_endstops(void);

#endif /* HYDRA_MCU_GPIO_H */
