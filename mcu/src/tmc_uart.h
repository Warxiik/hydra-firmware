#ifndef HYDRA_MCU_TMC_UART_H
#define HYDRA_MCU_TMC_UART_H

#include <stdint.h>
#include <stdbool.h>

void tmc_uart_init(void);
bool tmc_uart_write_reg(uint8_t bus, uint8_t addr, uint8_t reg, uint32_t value);
bool tmc_uart_read_reg(uint8_t bus, uint8_t addr, uint8_t reg, uint32_t *value);

#endif /* HYDRA_MCU_TMC_UART_H */
