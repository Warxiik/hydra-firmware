#include "transport.h"
#include "pins.h"
#include "config.h"
#include "protocol_defs.h"
#include "hardware/spi.h"
#include "hardware/gpio.h"

/*
 * Frame format (HYDRA_FRAME_OVERHEAD = 7):
 *   [SYNC0][SYNC1][SEQ][LEN_LO][LEN_HI][PAYLOAD...][CRC_LO][CRC_HI]
 *
 * CRC-16 CCITT covers SEQ + LEN + PAYLOAD.
 */

typedef enum {
    RX_SYNC0,
    RX_SYNC1,
    RX_HEADER,
    RX_PAYLOAD,
    RX_CRC,
} rx_state_t;

static uint8_t rx_buf[HYDRA_MAX_FRAME];
static uint8_t tx_buf[HYDRA_MAX_FRAME];
static uint8_t rx_payload[HYDRA_MAX_PAYLOAD];
static uint16_t rx_payload_len;
static volatile bool rx_ready;    /* A complete validated frame is available */

static rx_state_t rx_state;
static uint16_t rx_pos;
static uint16_t rx_expected_len;

static uint8_t tx_seq;

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
    spi_init(SPI_INSTANCE, SPI_BAUDRATE);
    spi_set_slave(SPI_INSTANCE, true);

    gpio_set_function(PIN_SPI_RX,  GPIO_FUNC_SPI);
    gpio_set_function(PIN_SPI_CSN, GPIO_FUNC_SPI);
    gpio_set_function(PIN_SPI_SCK, GPIO_FUNC_SPI);
    gpio_set_function(PIN_SPI_TX,  GPIO_FUNC_SPI);

    rx_state = RX_SYNC0;
    rx_pos = 0;
    rx_ready = false;
    tx_seq = 0x10; /* Response sequences start at 0x10 */
}

void transport_poll(void) {
    /* Read all available bytes from SPI FIFO */
    while (spi_is_readable(SPI_INSTANCE)) {
        uint8_t byte;
        spi_read_blocking(SPI_INSTANCE, 0x00, &byte, 1);

        switch (rx_state) {
        case RX_SYNC0:
            if (byte == HYDRA_SYNC_BYTE_0) {
                rx_buf[0] = byte;
                rx_state = RX_SYNC1;
            }
            break;

        case RX_SYNC1:
            if (byte == HYDRA_SYNC_BYTE_1) {
                rx_buf[1] = byte;
                rx_pos = 2;
                rx_state = RX_HEADER;
            } else {
                rx_state = RX_SYNC0; /* Bad sync — restart */
            }
            break;

        case RX_HEADER:
            rx_buf[rx_pos++] = byte;
            if (rx_pos == 5) { /* SEQ + LEN_LO + LEN_HI received */
                rx_expected_len = rx_buf[3] | ((uint16_t)rx_buf[4] << 8);
                if (rx_expected_len > HYDRA_MAX_PAYLOAD) {
                    rx_state = RX_SYNC0; /* Bad length — restart */
                } else if (rx_expected_len == 0) {
                    rx_state = RX_CRC; /* No payload, go straight to CRC */
                } else {
                    rx_state = RX_PAYLOAD;
                }
            }
            break;

        case RX_PAYLOAD:
            rx_buf[rx_pos++] = byte;
            if (rx_pos == 5 + rx_expected_len) {
                rx_state = RX_CRC;
            }
            break;

        case RX_CRC:
            rx_buf[rx_pos++] = byte;
            if (rx_pos == 5 + rx_expected_len + 2) {
                /* Full frame received — validate CRC */
                /* CRC covers bytes 2..end-2 (seq + len + payload) */
                uint16_t data_len = 3 + rx_expected_len; /* seq(1) + len(2) + payload */
                uint16_t computed = crc16_ccitt(&rx_buf[2], data_len);
                uint16_t received = rx_buf[rx_pos - 2] | ((uint16_t)rx_buf[rx_pos - 1] << 8);

                if (computed == received && !rx_ready) {
                    /* Copy payload out */
                    for (uint16_t i = 0; i < rx_expected_len; i++) {
                        rx_payload[i] = rx_buf[5 + i];
                    }
                    rx_payload_len = rx_expected_len;
                    rx_ready = true;
                }
                /* Either way, reset for next frame */
                rx_state = RX_SYNC0;
                rx_pos = 0;
            }
            break;
        }
    }
}

bool transport_send(const uint8_t *payload, uint16_t len) {
    if (len > HYDRA_MAX_PAYLOAD) return false;

    uint16_t pos = 0;

    /* Sync bytes */
    tx_buf[pos++] = HYDRA_SYNC_BYTE_0;
    tx_buf[pos++] = HYDRA_SYNC_BYTE_1;

    /* Sequence number */
    tx_buf[pos++] = tx_seq++;

    /* Length (little-endian) */
    tx_buf[pos++] = len & 0xFF;
    tx_buf[pos++] = (len >> 8) & 0xFF;

    /* Payload */
    for (uint16_t i = 0; i < len; i++) {
        tx_buf[pos++] = payload[i];
    }

    /* CRC-16 over seq + len + payload */
    uint16_t crc = crc16_ccitt(&tx_buf[2], 3 + len);
    tx_buf[pos++] = crc & 0xFF;
    tx_buf[pos++] = (crc >> 8) & 0xFF;

    spi_write_blocking(SPI_INSTANCE, tx_buf, pos);
    return true;
}

uint16_t transport_receive(uint8_t *buf, uint16_t buf_size) {
    if (!rx_ready) return 0;

    uint16_t len = rx_payload_len;
    if (len > buf_size) len = buf_size;

    for (uint16_t i = 0; i < len; i++) {
        buf[i] = rx_payload[i];
    }

    rx_ready = false;
    return len;
}
