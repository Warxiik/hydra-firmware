#include "transport.h"
#include "pins.h"
#include "config.h"
#include "protocol_defs.h"
#include "hardware/spi.h"
#include "hardware/gpio.h"

static uint8_t rx_buf[HYDRA_MAX_FRAME];
static uint8_t tx_buf[HYDRA_MAX_FRAME];
static uint16_t rx_pos;

/* CRC-16 CCITT lookup (generated at init or compile-time) */
static uint16_t crc16_ccitt(const uint8_t *data, uint16_t len) {
    uint16_t crc = 0xFFFF;
    for (uint16_t i = 0; i < len; i++) {
        crc ^= (uint16_t)data[i] << 8;
        for (uint8_t j = 0; j < 8; j++) {
            if (crc & 0x8000)
                crc = (crc << 1) ^ 0x1021;
            else
                crc <<= 1;
        }
    }
    return crc;
}

void transport_init(void) {
    /* Configure SPI0 as slave */
    spi_init(SPI_INSTANCE, SPI_BAUDRATE);
    spi_set_slave(SPI_INSTANCE, true);

    gpio_set_function(PIN_SPI_RX,  GPIO_FUNC_SPI);
    gpio_set_function(PIN_SPI_CSN, GPIO_FUNC_SPI);
    gpio_set_function(PIN_SPI_SCK, GPIO_FUNC_SPI);
    gpio_set_function(PIN_SPI_TX,  GPIO_FUNC_SPI);

    rx_pos = 0;
}

void transport_poll(void) {
    /* Read available bytes from SPI */
    while (spi_is_readable(SPI_INSTANCE) && rx_pos < HYDRA_MAX_FRAME) {
        spi_read_blocking(SPI_INSTANCE, 0x00, &rx_buf[rx_pos], 1);
        rx_pos++;
    }

    /* TODO: Detect frame boundaries using sync bytes and length field */
}

bool transport_send(const uint8_t *payload, uint16_t len) {
    if (len > HYDRA_MAX_PAYLOAD) return false;

    uint16_t pos = 0;

    /* Sync bytes */
    tx_buf[pos++] = HYDRA_SYNC_BYTE_0;
    tx_buf[pos++] = HYDRA_SYNC_BYTE_1;

    /* Sequence number (TODO: increment) */
    tx_buf[pos++] = 0x10;

    /* Length (little-endian) */
    tx_buf[pos++] = len & 0xFF;
    tx_buf[pos++] = (len >> 8) & 0xFF;

    /* Payload */
    for (uint16_t i = 0; i < len; i++) {
        tx_buf[pos++] = payload[i];
    }

    /* CRC-16 over sequence + length + payload */
    uint16_t crc = crc16_ccitt(&tx_buf[2], pos - 2);
    tx_buf[pos++] = crc & 0xFF;
    tx_buf[pos++] = (crc >> 8) & 0xFF;

    /* Write to SPI TX */
    spi_write_blocking(SPI_INSTANCE, tx_buf, pos);

    return true;
}

uint16_t transport_receive(uint8_t *buf, uint16_t buf_size) {
    /* TODO: Parse framed messages from rx_buf, validate CRC, return payload */
    (void)buf;
    (void)buf_size;
    return 0;
}
