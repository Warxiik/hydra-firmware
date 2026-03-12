#include "tmc_uart.h"
#include "pins.h"
#include "config.h"
#include "hardware/pio.h"
#include "tmc_uart.pio.h"

static PIO tmc_pio;
static uint tmc_tx_sm[2]; /* Two UART buses */
static uint tmc_rx_sm[2];

void tmc_uart_init(void) {
    tmc_pio = pio1; /* Use PIO1 (PIO0 is for steppers) */

    /* Load TX and RX programs */
    uint tx_offset = pio_add_program(tmc_pio, &tmc_uart_tx_program);
    uint rx_offset = pio_add_program(tmc_pio, &tmc_uart_rx_program);

    /* Bus 0: steppers 0-3 (TMC addresses 0-3) */
    tmc_tx_sm[0] = pio_claim_unused_sm(tmc_pio, true);
    tmc_uart_tx_program_init(tmc_pio, tmc_tx_sm[0], tx_offset, PIN_TMC_UART_0, TMC_UART_BAUD);

    tmc_rx_sm[0] = pio_claim_unused_sm(tmc_pio, true);
    tmc_uart_rx_program_init(tmc_pio, tmc_rx_sm[0], rx_offset, PIN_TMC_UART_0, TMC_UART_BAUD);

    /* Bus 1: steppers 4-6 (TMC addresses 0-2) */
    tmc_tx_sm[1] = pio_claim_unused_sm(tmc_pio, true);
    tmc_uart_tx_program_init(tmc_pio, tmc_tx_sm[1], tx_offset, PIN_TMC_UART_1, TMC_UART_BAUD);

    tmc_rx_sm[1] = pio_claim_unused_sm(tmc_pio, true);
    tmc_uart_rx_program_init(tmc_pio, tmc_rx_sm[1], rx_offset, PIN_TMC_UART_1, TMC_UART_BAUD);
}

bool tmc_uart_write_reg(uint8_t bus, uint8_t addr, uint8_t reg, uint32_t value) {
    if (bus > 1 || addr > 3) return false;

    /* TMC2209 write datagram: [SYNC(0x05)] [ADDR] [REG|0x80] [DATA(4)] [CRC] */
    /* TODO: Build and send TMC2209 write datagram via PIO TX */
    (void)reg;
    (void)value;

    return true;
}

bool tmc_uart_read_reg(uint8_t bus, uint8_t addr, uint8_t reg, uint32_t *value) {
    if (bus > 1 || addr > 3) return false;

    /* TMC2209 read: send read request, then switch to RX and receive response */
    /* TODO: Implement TMC2209 read sequence */
    (void)reg;
    (void)value;

    return false;
}
