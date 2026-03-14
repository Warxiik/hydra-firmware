#include "tmc_uart.h"
#include "pins.h"
#include "config.h"
#include "hardware/pio.h"
#include "pico/time.h"
#include "tmc_uart.pio.h"

static PIO tmc_pio;
static uint tmc_tx_sm[2]; /* Two UART buses */
static uint tmc_rx_sm[2];

/**
 * TMC2209 CRC-8 calculation.
 * Polynomial: x^8 + x^2 + x + 1 (0x07).
 * Processes each byte LSB-first.
 */
static uint8_t tmc_crc8(const uint8_t *data, uint8_t len) {
    uint8_t crc = 0;
    for (uint8_t i = 0; i < len; i++) {
        uint8_t byte = data[i];
        for (uint8_t j = 0; j < 8; j++) {
            if ((crc >> 7) ^ (byte & 0x01)) {
                crc = (crc << 1) ^ 0x07;
            } else {
                crc <<= 1;
            }
            byte >>= 1;
        }
    }
    return crc;
}

/**
 * Compute byte transmission time in microseconds at the configured baud rate.
 * Each UART byte = 1 start + 8 data + 1 stop = 10 bits.
 */
static uint32_t byte_time_us(void) {
    return (10 * 1000000) / TMC_UART_BAUD;
}

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

    /*
     * TMC2209 write datagram (8 bytes):
     * [SYNC=0x05] [SLAVE_ADDR] [REG|0x80] [DATA31:24] [DATA23:16] [DATA15:8] [DATA7:0] [CRC]
     */
    uint8_t datagram[8];
    datagram[0] = 0x05;
    datagram[1] = addr;
    datagram[2] = reg | 0x80;
    datagram[3] = (value >> 24) & 0xFF;
    datagram[4] = (value >> 16) & 0xFF;
    datagram[5] = (value >> 8) & 0xFF;
    datagram[6] = value & 0xFF;
    datagram[7] = tmc_crc8(datagram, 7);

    /* Push each byte to the PIO TX FIFO */
    for (int i = 0; i < 8; i++) {
        pio_sm_put_blocking(tmc_pio, tmc_tx_sm[bus], (uint32_t)datagram[i]);
    }

    /* Wait for all 8 bytes to finish transmitting */
    sleep_us(8 * byte_time_us() + 100);

    return true;
}

bool tmc_uart_read_reg(uint8_t bus, uint8_t addr, uint8_t reg, uint32_t *value) {
    if (bus > 1 || addr > 3 || !value) return false;

    /*
     * TMC2209 read sequence:
     * 1. Send read request (4 bytes): [SYNC=0x05] [SLAVE_ADDR] [REG] [CRC]
     * 2. Wait for line turnaround
     * 3. Receive response (8 bytes): [SYNC=0x05] [0xFF] [REG] [DATA31:24..DATA7:0] [CRC]
     */

    /* Build read request */
    uint8_t request[4];
    request[0] = 0x05;
    request[1] = addr;
    request[2] = reg;
    request[3] = tmc_crc8(request, 3);

    /* Send request via PIO TX */
    for (int i = 0; i < 4; i++) {
        pio_sm_put_blocking(tmc_pio, tmc_tx_sm[bus], (uint32_t)request[i]);
    }

    /* Wait for request to finish transmitting */
    sleep_us(4 * byte_time_us() + 50);

    /* Flush any stale data from RX FIFO */
    while (!pio_sm_is_rx_fifo_empty(tmc_pio, tmc_rx_sm[bus])) {
        (void)pio_sm_get(tmc_pio, tmc_rx_sm[bus]);
    }

    /* Enable RX state machine for response capture */
    pio_sm_set_enabled(tmc_pio, tmc_rx_sm[bus], true);

    /* Read 8 response bytes with timeout */
    uint8_t response[8];
    for (int i = 0; i < 8; i++) {
        uint32_t timeout_us = 5000; /* 5ms per byte max */
        while (pio_sm_is_rx_fifo_empty(tmc_pio, tmc_rx_sm[bus])) {
            if (timeout_us == 0) {
                pio_sm_set_enabled(tmc_pio, tmc_rx_sm[bus], false);
                return false;
            }
            sleep_us(1);
            timeout_us--;
        }
        response[i] = (uint8_t)(pio_sm_get(tmc_pio, tmc_rx_sm[bus]) & 0xFF);
    }

    /* Disable RX state machine */
    pio_sm_set_enabled(tmc_pio, tmc_rx_sm[bus], false);

    /* Validate sync byte */
    if (response[0] != 0x05) return false;

    /* Validate CRC */
    if (tmc_crc8(response, 7) != response[7]) return false;

    /* Extract 32-bit register value (big-endian in datagram) */
    *value = ((uint32_t)response[3] << 24)
           | ((uint32_t)response[4] << 16)
           | ((uint32_t)response[5] << 8)
           | ((uint32_t)response[6]);

    return true;
}
