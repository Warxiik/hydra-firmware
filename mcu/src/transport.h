#ifndef HYDRA_MCU_TRANSPORT_H
#define HYDRA_MCU_TRANSPORT_H

#include <stdint.h>
#include <stdbool.h>

/* Initialize SPI slave transport */
void transport_init(void);

/* Poll SPI for incoming frames (non-blocking) */
void transport_poll(void);

/* Send a response frame to host */
bool transport_send(const uint8_t *payload, uint16_t len);

/* Get next received payload (returns length, 0 if none) */
uint16_t transport_receive(uint8_t *buf, uint16_t buf_size);

#endif /* HYDRA_MCU_TRANSPORT_H */
