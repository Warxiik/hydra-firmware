#ifndef HYDRA_PROTOCOL_DEFS_H
#define HYDRA_PROTOCOL_DEFS_H

/**
 * Hydra host <-> MCU binary protocol definitions.
 * This file is shared between host (C++) and MCU (C) code.
 * Keep it C-compatible.
 */

#include <stdint.h>

/* ── Transport framing ─────────────────────────────────────────────── */

#define HYDRA_SYNC_BYTE_0  0x48  /* 'H' */
#define HYDRA_SYNC_BYTE_1  0x59  /* 'Y' */
#define HYDRA_MAX_PAYLOAD  56
#define HYDRA_FRAME_OVERHEAD 7   /* sync(2) + seq(1) + len(2) + crc(2) */
#define HYDRA_MAX_FRAME    (HYDRA_MAX_PAYLOAD + HYDRA_FRAME_OVERHEAD)

/* ── Host -> MCU command IDs ───────────────────────────────────────── */

#define CMD_NOP               0x00  /* Heartbeat / keepalive              */
#define CMD_GET_CLOCK         0x01  /* Request MCU clock value             */
#define CMD_GET_STATUS        0x02  /* Request full status report          */
#define CMD_EMERGENCY_STOP    0x03  /* Immediate halt, kill heaters        */
#define CMD_RESET             0x04  /* Soft reset MCU                      */

#define CMD_CONFIG_STEPPER    0x10  /* Configure stepper channel           */
#define CMD_QUEUE_STEP        0x11  /* Queue step segment {oid,int,cnt,add}*/
#define CMD_SET_NEXT_STEP_DIR 0x12  /* Set direction for next segment      */
#define CMD_RESET_STEP_CLOCK  0x13  /* Reset step clock for a stepper      */
#define CMD_STEPPER_GET_POS   0x14  /* Query accumulated step count        */

#define CMD_ENDSTOP_HOME      0x18  /* Begin homing on endstop pin         */
#define CMD_ENDSTOP_QUERY     0x19  /* Query endstop state                 */

#define CMD_SET_PWM           0x20  /* Set PWM duty {channel, duty_u16}    */
#define CMD_SET_DIGITAL       0x21  /* Set digital output {pin, value}     */

#define CMD_TMC_WRITE         0x30  /* Write TMC2209 register              */
#define CMD_TMC_READ          0x31  /* Read TMC2209 register               */

/* ── MCU -> Host response IDs ─────────────────────────────────────── */

#define RSP_CLOCK             0x81  /* Clock value response                */
#define RSP_STATUS            0x82  /* Periodic status report              */
#define RSP_ACK               0x83  /* Command acknowledgment              */
#define RSP_STEPPER_POS       0x84  /* Step position response              */
#define RSP_ENDSTOP_STATE     0x88  /* Endstop triggered notification      */
#define RSP_TMC_READ          0x91  /* TMC register read response          */
#define RSP_ERROR             0xFF  /* Error report                        */

/* ── Stepper channels ─────────────────────────────────────────────── */

#define STEPPER_COREXY_A      0    /* CoreXY motor A (X+Y)                */
#define STEPPER_COREXY_B      1    /* CoreXY motor B (X-Y)                */
#define STEPPER_OFFSET_X      2    /* Nozzle 1 relative X offset          */
#define STEPPER_OFFSET_Y      3    /* Nozzle 1 relative Y offset          */
#define STEPPER_Z             4    /* Bed Z axis                          */
#define STEPPER_EXTRUDER_0    5    /* Nozzle 0 filament drive             */
#define STEPPER_EXTRUDER_1    6    /* Nozzle 1 filament drive             */
#define STEPPER_COUNT         7

/* ── PWM channels ─────────────────────────────────────────────────── */

#define PWM_HEATER_NOZZLE_0   0
#define PWM_HEATER_NOZZLE_1   1
#define PWM_HEATER_BED        2
#define PWM_FAN_PART_0        3
#define PWM_FAN_PART_1        4
#define PWM_FAN_HOTEND_0      5
#define PWM_FAN_HOTEND_1      6
#define PWM_CHANNEL_COUNT     7

/* ── ADC channels ─────────────────────────────────────────────────── */

#define ADC_THERM_NOZZLE_0    0
#define ADC_THERM_NOZZLE_1    1
#define ADC_THERM_BED         2
#define ADC_CHANNEL_COUNT     3

/* ── Status report structure ──────────────────────────────────────── */

typedef struct __attribute__((packed)) {
    uint32_t mcu_clock;                     /* Current MCU clock ticks       */
    uint16_t adc_raw[ADC_CHANNEL_COUNT];    /* Raw 12-bit ADC readings       */
    uint8_t  endstop_state;                 /* Endstop bitmask (1=triggered) */
    uint8_t  queue_depth[STEPPER_COUNT];    /* Per-stepper queue fill (0-255)*/
    uint8_t  flags;                         /* Error/status flags            */
} hydra_status_report_t;

/* Status flags */
#define STATUS_FLAG_OVERTEMP   (1 << 0)  /* MCU-side thermal cutoff active  */
#define STATUS_FLAG_ESTOP      (1 << 1)  /* Emergency stop active           */
#define STATUS_FLAG_WATCHDOG   (1 << 2)  /* Watchdog timeout occurred       */
#define STATUS_FLAG_QUEUE_FULL (1 << 3)  /* At least one step queue full    */

/* ── Queue step command ───────────────────────────────────────────── */

typedef struct __attribute__((packed)) {
    uint8_t  oid;          /* Stepper channel (0-6)                        */
    uint32_t interval;     /* Clock ticks for first step interval          */
    uint16_t count;        /* Number of steps in this segment              */
    int16_t  add;          /* Interval adjustment per step (linear ramp)   */
} hydra_queue_step_t;

/*
 * Step timing model:
 *   step[0] at: reference_clock + interval
 *   step[i] at: step[i-1] + interval + add * i
 *
 * This encodes constant velocity (add=0), acceleration (add<0),
 * and deceleration (add>0) as a single compact command.
 */

/* ── Error codes ──────────────────────────────────────────────────── */

#define ERR_NONE              0x00
#define ERR_BAD_CMD           0x01  /* Unknown command ID                  */
#define ERR_BAD_CRC           0x02  /* CRC mismatch                       */
#define ERR_QUEUE_OVERFLOW    0x03  /* Step queue full, command dropped    */
#define ERR_OVERTEMP          0x04  /* Thermal limit exceeded              */
#define ERR_WATCHDOG          0x05  /* Host heartbeat timeout              */
#define ERR_ENDSTOP_HIT       0x06  /* Unexpected endstop trigger          */
#define ERR_TMC_COMM          0x07  /* TMC2209 UART communication failure  */

#endif /* HYDRA_PROTOCOL_DEFS_H */
