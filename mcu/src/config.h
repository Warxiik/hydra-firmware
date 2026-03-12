#ifndef HYDRA_MCU_CONFIG_H
#define HYDRA_MCU_CONFIG_H

/* ── SPI configuration ───────────────────────────────────────────── */
#define SPI_INSTANCE       spi0
#define SPI_BAUDRATE       4000000   /* 4 MHz */

/* ── Step queue ──────────────────────────────────────────────────── */
#define STEP_QUEUE_SIZE    256       /* Segments per stepper channel */

/* ── ADC ─────────────────────────────────────────────────────────── */
#define ADC_SAMPLE_HZ      100      /* Thermistor sample rate */

/* ── Heater PWM ──────────────────────────────────────────────────── */
#define HEATER_PWM_HZ      10       /* Slow PWM for heater MOSFETs */
#define BED_PWM_HZ         1        /* Very slow PWM for bed SSR */

/* ── Fan PWM ─────────────────────────────────────────────────────── */
#define FAN_PWM_HZ         25000    /* 25kHz for silent fan operation */

/* ── Safety limits (MCU-side hard cutoffs) ────────────────────────── */
#define MAX_TEMP_NOZZLE_C  300      /* Emergency cutoff temperature */
#define MAX_TEMP_BED_C     120      /* Emergency cutoff temperature */

/* ── Watchdog ────────────────────────────────────────────────────── */
#define WATCHDOG_TIMEOUT_MS 500     /* Kill heaters if host silent */

/* ── Status report ───────────────────────────────────────────────── */
#define STATUS_REPORT_HZ   100      /* Send status to host at 100Hz */

/* ── TMC2209 ─────────────────────────────────────────────────────── */
#define TMC_UART_BAUD      115200

#endif /* HYDRA_MCU_CONFIG_H */
