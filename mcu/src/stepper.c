#include "stepper.h"
#include "pins.h"
#include "hardware/pio.h"
#include "hardware/gpio.h"
#include "step_dir_sr.pio.h"

static stepper_channel_t channels[STEPPER_COUNT];
static PIO step_pio;
static uint step_sm;

void stepper_init(void) {
    /* Clear all channels */
    for (int i = 0; i < STEPPER_COUNT; i++) {
        channels[i].head = 0;
        channels[i].tail = 0;
        channels[i].dir = 0;
        channels[i].position = 0;
    }

    /* Configure stepper enable pin (active low) */
    gpio_init(PIN_STEPPER_EN);
    gpio_set_dir(PIN_STEPPER_EN, GPIO_OUT);
    gpio_put(PIN_STEPPER_EN, 1); /* Disabled at startup */

    /* Configure shift register pins */
    gpio_init(PIN_SR_DATA);
    gpio_init(PIN_SR_CLOCK);
    gpio_init(PIN_SR_LATCH);
    gpio_set_dir(PIN_SR_DATA, GPIO_OUT);
    gpio_set_dir(PIN_SR_CLOCK, GPIO_OUT);
    gpio_set_dir(PIN_SR_LATCH, GPIO_OUT);
}

void stepper_core1_init(void) {
    /* Load PIO program for shift-register step generation */
    step_pio = pio0;
    uint offset = pio_add_program(step_pio, &step_dir_sr_program);
    step_sm = pio_claim_unused_sm(step_pio, true);

    /* TODO: Initialize PIO state machine with pin configs */
    /* step_dir_sr_program_init(step_pio, step_sm, offset, PIN_SR_DATA, PIN_SR_CLOCK, PIN_SR_LATCH); */
}

void stepper_core1_loop(void) {
    /*
     * Main step generation loop:
     * 1. Check all channel queues for pending segments
     * 2. Calculate next step time for each channel
     * 3. Build 16-bit shift register word with step/dir bits
     * 4. Push to PIO at the correct time
     *
     * TODO: Implement DDA-style step scheduling across all channels
     */

    /* Placeholder: tight loop checking queues */
    tight_loop_contents();
}

bool stepper_queue(uint8_t channel, const hydra_queue_step_t *cmd) {
    if (channel >= STEPPER_COUNT) return false;

    stepper_channel_t *ch = &channels[channel];
    uint16_t next_head = (ch->head + 1) % STEP_QUEUE_SIZE;

    /* Queue full? */
    if (next_head == ch->tail) return false;

    ch->buf[ch->head].interval = cmd->interval;
    ch->buf[ch->head].count = cmd->count;
    ch->buf[ch->head].add = cmd->add;
    __dmb(); /* Memory barrier before publishing */
    ch->head = next_head;

    return true;
}

void stepper_set_dir(uint8_t channel, uint8_t dir) {
    if (channel < STEPPER_COUNT) {
        channels[channel].dir = dir ? 1 : 0;
    }
}

void stepper_reset_clock(uint8_t channel, uint32_t clock) {
    /* TODO: Set reference clock for the channel's step timing */
    (void)channel;
    (void)clock;
}

int32_t stepper_get_position(uint8_t channel) {
    if (channel >= STEPPER_COUNT) return 0;
    return channels[channel].position;
}

uint8_t stepper_queue_depth(uint8_t channel) {
    if (channel >= STEPPER_COUNT) return 0;
    stepper_channel_t *ch = &channels[channel];
    int16_t depth = (int16_t)ch->head - (int16_t)ch->tail;
    if (depth < 0) depth += STEP_QUEUE_SIZE;
    /* Scale to 0-255 */
    return (uint8_t)((depth * 255) / STEP_QUEUE_SIZE);
}

void stepper_emergency_stop(void) {
    /* Flush all queues */
    for (int i = 0; i < STEPPER_COUNT; i++) {
        channels[i].head = 0;
        channels[i].tail = 0;
    }
    /* Disable drivers */
    stepper_enable(false);
}

void stepper_enable(bool enable) {
    gpio_put(PIN_STEPPER_EN, enable ? 0 : 1); /* Active low */
}
