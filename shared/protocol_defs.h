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

#define CMD_TMC_WRITE         0x28  /* Write TMC2209 register              */
#define CMD_TMC_READ          0x29  /* Read TMC2209 register               */

#define CMD_VALVE_SET         0x30  /* Set valve bitmask {mask_u8}         */
#define CMD_VALVE_GET         0x31  /* Query current valve state            */

/* ── MCU -> Host response IDs ─────────────────────────────────────── */

#define RSP_CLOCK             0x81  /* Clock value response                */
#define RSP_STATUS            0x82  /* Periodic status report              */
#define RSP_ACK               0x83  /* Command acknowledgment              */
#define RSP_STEPPER_POS       0x84  /* Step position response              */
#define RSP_ENDSTOP_STATE     0x88  /* Endstop triggered notification      */
#define RSP_TMC_READ          0x91  /* TMC register read response          */
#define RSP_VALVE_STATE       0x92  /* Current valve bitmask               */
#define RSP_ERROR             0xFF  /* Error report                        */

/* ── Stepper channels ─────────────────────────────────────────────── */

#define STEPPER_COREXY_A      0    /* CoreXY motor A (X+Y)                */
#define STEPPER_COREXY_B      1    /* CoreXY motor B (X-Y)                */
#define STEPPER_Z1            2    /* Bed Z axis — lead screw 1           */
#define STEPPER_Z2            3    /* Bed Z axis — lead screw 2           */
#define STEPPER_Z3            4    /* Bed Z axis — lead screw 3           */
#define STEPPER_EXTRUDER      5    /* Filament drive (shared melt chamber)*/
#define STEPPER_SPARE         6    /* Spare / future use                  */
#define STEPPER_COUNT         7

/* Valve nozzle bitmask bits */
#define VALVE_NOZZLE_0        (1 << 0)  /* 0.4mm quality nozzle            */
#define VALVE_NOZZLE_1        (1 << 1)  /* 0.6mm nozzle                    */
#define VALVE_NOZZLE_2        (1 << 2)  /* 0.6mm nozzle                    */
#define VALVE_NOZZLE_3        (1 << 3)  /* 0.8mm nozzle                    */
#define VALVE_COUNT           4
#define VALVE_ALL             0x0F
#define VALVE_QUALITY         VALVE_NOZZLE_0
#define VALVE_INFILL          (VALVE_NOZZLE_1 | VALVE_NOZZLE_2 | VALVE_NOZZLE_3)

/* ── PWM channels ─────────────────────────────────────────────────── */

#define PWM_HEATER_MANIFOLD   0    /* Shared melt chamber heater          */
#define PWM_HEATER_BED        1
#define PWM_FAN_PART          2    /* Part cooling fan                    */
#define PWM_FAN_HOTEND        3    /* Hotend heatsink fan                 */
#define PWM_CHANNEL_COUNT     4

/* ── ADC channels ─────────────────────────────────────────────────── */

#define ADC_THERM_MANIFOLD    0    /* Shared manifold thermistor          */
#define ADC_THERM_BED         1
#define ADC_CHANNEL_COUNT     2

/* ── Status report structure ──────────────────────────────────────── */

typedef struct __attribute__((packed)) {
    uint32_t mcu_clock;                     /* Current MCU clock ticks       */
    uint16_t adc_raw[ADC_CHANNEL_COUNT];    /* Raw 12-bit ADC readings       */
    uint8_t  endstop_state;                 /* Endstop bitmask (1=triggered) */
    uint8_t  queue_depth[STEPPER_COUNT];    /* Per-stepper queue fill (0-255)*/
    uint8_t  valve_state;                   /* Current valve bitmask         */
    uint8_t  flags;                         /* Error/status flags            */
} hydra_status_report_t;

/* Status flags */
#define STATUS_FLAG_OVERTEMP   (1 << 0)  /* MCU-side thermal cutoff active  */
#define STATUS_FLAG_ESTOP      (1 << 1)  /* Emergency stop active           */
#define STATUS_FLAG_WATCHDOG   (1 << 2)  /* Watchdog timeout occurred       */
#define STATUS_FLAG_QUEUE_FULL (1 << 3)  /* At least one step queue full    */

/* ── Queue step command ───────────────────────────────────────────── */

typedef struct __attribute__((packed)) {
    uint8_t  oid;          /* Stepper channel (0-6), see STEPPER_* defines  */
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
