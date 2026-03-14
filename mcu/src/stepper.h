#ifndef HYDRA_MCU_STEPPER_H
#define HYDRA_MCU_STEPPER_H

#include <stdint.h>
#include <stdbool.h>
#include "protocol_defs.h"
#include "config.h"

/* Step queue entry — matches Klipper's interval/count/add model */
typedef struct {
    uint32_t interval;  /* Clock ticks for first step interval */
    uint16_t count;     /* Number of steps in this segment     */
    int16_t  add;       /* Interval change per step            */
} step_segment_t;

/* Per-channel step queue */
typedef struct {
    step_segment_t buf[STEP_QUEUE_SIZE];
    volatile uint16_t head;  /* Written by Core 0 (SPI rx) */
    volatile uint16_t tail;  /* Read by Core 1 (PIO feed)  */
    uint8_t  dir;            /* Current direction (0 or 1)  */
    int32_t  position;       /* Accumulated step count      */
} stepper_channel_t;

/* Initialize stepper PIO and queues */
void stepper_init(void);

/* Core 1 init — claim PIO state machines */
void stepper_core1_init(void);

/* Core 1 main loop — drain queues, feed PIO */
void stepper_core1_loop(void);

/* Enqueue a step segment (called from Core 0) */
bool stepper_queue(uint8_t channel, const hydra_queue_step_t *cmd);

/* Set direction for next segment */
void stepper_set_dir(uint8_t channel, uint8_t dir);

/* Reset step clock reference for a channel */
void stepper_reset_clock(uint8_t channel, uint32_t clock);

/* Get current step position */
int32_t stepper_get_position(uint8_t channel);

/* Get queue depth (0-255, for status report) */
uint8_t stepper_queue_depth(uint8_t channel);

/* Emergency stop — flush all queues, disable steppers */
void stepper_emergency_stop(void);

/* Enable/disable all stepper drivers */
void stepper_enable(bool enable);

/**
 * Endstop-triggered homing support.
 * When a channel is in homing mode, the step scheduler monitors the
 * specified endstop bit and halts the channel when triggered.
 *
 * @param channel     Stepper channel (0-6)
 * @param endstop_bit Endstop bitmask to monitor (0 = disable homing mode)
 */
void stepper_set_homing(uint8_t channel, uint8_t endstop_bit);

/**
 * Check if a channel's homing sequence triggered (endstop hit).
 * Clears the flag after reading.
 */
bool stepper_homing_triggered(uint8_t channel);

/**
 * Flush the step queue for a single channel.
 */
void stepper_flush_channel(uint8_t channel);

#endif /* HYDRA_MCU_STEPPER_H */
