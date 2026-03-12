#ifndef HYDRA_MCU_PINS_H
#define HYDRA_MCU_PINS_H

/**
 * RP2040 pin assignments for the Hydra carrier board.
 *
 * Total GPIO used: 23 of 30 available.
 * Remaining: GP21, GP24, GP25, GP26 (GP26 is ADC0, reserved as spare ADC).
 */

/* ── SPI slave (host communication) ──────────────────────────────── */
#define PIN_SPI_RX     0   /* GP0  — MISO (MCU -> host)  SPI0 */
#define PIN_SPI_CSN    1   /* GP1  — CS (active low)     SPI0 */
#define PIN_SPI_SCK    2   /* GP2  — Clock               SPI0 */
#define PIN_SPI_TX     3   /* GP3  — MOSI (host -> MCU)  SPI0 */

/* ── Shift register (step/dir output for 7 axes) ────────────────── */
#define PIN_SR_DATA    4   /* GP4  — 74HC595 serial data (SER)    */
#define PIN_SR_CLOCK   5   /* GP5  — 74HC595 shift clock (SRCLK)  */
#define PIN_SR_LATCH   6   /* GP6  — 74HC595 storage clock (RCLK) */

/* ── TMC2209 UART buses (PIO single-wire half-duplex) ────────────── */
#define PIN_TMC_UART_0 7   /* GP7  — TMC bus 0 (steppers 0-3, addressed via AD0/AD1) */
#define PIN_TMC_UART_1 8   /* GP8  — TMC bus 1 (steppers 4-6) */

/* ── Endstop inputs ──────────────────────────────────────────────── */
#define PIN_ENDSTOP_X  9   /* GP9  — X axis endstop      */
#define PIN_ENDSTOP_Y  10  /* GP10 — Y axis endstop      */
#define PIN_ENDSTOP_Z  11  /* GP11 — Z axis endstop (bed probe) */

/* ── Heater MOSFET outputs (slow PWM via software timer) ─────────── */
#define PIN_HEATER_N0  12  /* GP12 — Nozzle 0 heater     */
#define PIN_HEATER_N1  13  /* GP13 — Nozzle 1 heater     */
#define PIN_HEATER_BED 14  /* GP14 — Heated bed          */

/* ── Fan PWM outputs (hardware PWM, 25kHz for silence) ───────────── */
#define PIN_FAN_PART_0 15  /* GP15 — Part cooling fan 0  */
#define PIN_FAN_PART_1 16  /* GP16 — Part cooling fan 1  */
#define PIN_FAN_HE_0   17  /* GP17 — Hotend heatsink fan 0 */
#define PIN_FAN_HE_1   18  /* GP18 — Hotend heatsink fan 1 */

/* ── Status LED ──────────────────────────────────────────────────── */
#define PIN_LED_STATUS 19  /* GP19 — Status/error LED    */

/* ── Stepper enable (active low, shared for all TMC2209s) ────────── */
#define PIN_STEPPER_EN 20  /* GP20 — Global stepper enable */

/* ── Thermistor ADC inputs ───────────────────────────────────────── */
/* RP2040 ADC channels are on GP26-GP29 */
#define PIN_ADC_THERM_N0  27  /* GP27 — ADC1: Nozzle 0 thermistor */
#define PIN_ADC_THERM_N1  28  /* GP28 — ADC2: Nozzle 1 thermistor */
#define PIN_ADC_THERM_BED 29  /* GP29 — ADC3: Bed thermistor      */

/* ── Shift register bit mapping ──────────────────────────────────── */
/*
 * The 74HC595 outputs 16 bits (2 chained chips) as:
 *   [STEP6 DIR6 STEP5 DIR5 STEP4 DIR4 STEP3 DIR3  STEP2 DIR2 STEP1 DIR1 STEP0 DIR0 x x]
 *
 * Bits are shifted out MSB first, so bit 15 goes to Q7 of the first chip.
 */
#define SR_BIT_STEP(ch)  (15 - (ch) * 2)     /* Step bit position for channel ch */
#define SR_BIT_DIR(ch)   (15 - (ch) * 2 - 1) /* Dir bit position for channel ch  */

#endif /* HYDRA_MCU_PINS_H */
