#ifndef HYDRA_MCU_PROTOCOL_H
#define HYDRA_MCU_PROTOCOL_H

/* Initialize protocol handler */
void protocol_init(void);

/* Process received commands (called from Core 0 main loop) */
void protocol_process(void);

/* Send periodic status report to host (rate-limited internally) */
void protocol_send_status(void);

#endif /* HYDRA_MCU_PROTOCOL_H */
