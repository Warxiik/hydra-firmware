#include "stepper.h"
#include "pins.h"
#include "hardware/pio.h"
#include "hardware/gpio.h"
#include "hardware/timer.h"
#include "step_dir_sr.pio.h"

static stepper_channel_t channels[STEPPER_COUNT];
static PIO step_pio;
static uint step_sm;
static uint pio_offset;

/* Per-channel runtime state for the step scheduler */
typedef struct {
    uint32_t next_step_time;   /* Absolute time (us) of next step */
    uint32_t interval;         /* Current interval */
    int16_t  add;              /* Interval change per step */
    uint16_t steps_remaining;  /* Steps left in current segment */
    bool     active;           /* Currently executing a segment */
} step_state_t;

static step_state_t step_states[STEPPER_COUNT];

/* Per-channel homing state */
static uint8_t homing_endstop_bit[STEPPER_COUNT]; /* 0 = not homing */
static volatile bool homing_triggered[STEPPER_COUNT];

void stepper_init(void) {
    for (int i = 0; i < STEPPER_COUNT; i++) {
        channels[i].head = 0;
        channels[i].tail = 0;
        channels[i].dir = 0;
        channels[i].position = 0;
        step_states[i].active = false;
    }

    /* Configure stepper enable pin (active low) */
    gpio_init(PIN_STEPPER_EN);
    gpio_set_dir(PIN_STEPPER_EN, GPIO_OUT);
    gpio_put(PIN_STEPPER_EN, 1); /* Disabled at startup */

    /* Configure shift register pins (will be claimed by PIO later) */
    gpio_init(PIN_SR_DATA);
    gpio_init(PIN_SR_CLOCK);
    gpio_init(PIN_SR_LATCH);
    gpio_set_dir(PIN_SR_DATA, GPIO_OUT);
    gpio_set_dir(PIN_SR_CLOCK, GPIO_OUT);
    gpio_set_dir(PIN_SR_LATCH, GPIO_OUT);
}

void stepper_core1_init(void) {
    step_pio = pio0;
    pio_offset = pio_add_program(step_pio, &step_dir_sr_program);
    step_sm = pio_claim_unused_sm(step_pio, true);

    /* Configure PIO state machine for shift register output */
    pio_sm_config c = step_dir_sr_program_get_default_config(pio_offset);

    /* Set output pins: data pin, then clock is side-set */
    sm_config_set_out_pins(&c, PIN_SR_DATA, 1);
    sm_config_set_sideset_pins(&c, PIN_SR_CLOCK);
    sm_config_set_set_pins(&c, PIN_SR_LATCH, 1);

    /* Shift 16 bits out MSB first, auto-pull at 16 bits */
    sm_config_set_out_shift(&c, false, true, 16);

    /* Clock divider: 125MHz / 4 = 31.25MHz per PIO instruction */
    sm_config_set_clkdiv(&c, 4.0f);

    /* Initialize pin directions */
    pio_sm_set_consecutive_pindirs(step_pio, step_sm, PIN_SR_DATA, 1, true);
    pio_sm_set_consecutive_pindirs(step_pio, step_sm, PIN_SR_CLOCK, 1, true);
    pio_sm_set_consecutive_pindirs(step_pio, step_sm, PIN_SR_LATCH, 1, true);

    pio_gpio_init(step_pio, PIN_SR_DATA);
    pio_gpio_init(step_pio, PIN_SR_CLOCK);
    pio_gpio_init(step_pio, PIN_SR_LATCH);

    pio_sm_init(step_pio, step_sm, pio_offset, &c);
    pio_sm_set_enabled(step_pio, step_sm, true);
}

/*
 * Build a 16-bit shift register word from current step/dir state.
 * Bit layout (MSB first, 2 chained 74HC595):
 *   [STEP6 DIR6 STEP5 DIR5 STEP4 DIR4 STEP3 DIR3 STEP2 DIR2 STEP1 DIR1 STEP0 DIR0 x x]
 */
static uint16_t build_sr_word(bool step_bits[STEPPER_COUNT]) {
    uint16_t word = 0;
    for (int ch = 0; ch < STEPPER_COUNT; ch++) {
        if (step_bits[ch])
            word |= (1 << SR_BIT_STEP(ch));
        if (channels[ch].dir)
            word |= (1 << SR_BIT_DIR(ch));
    }
    return word;
}

void stepper_core1_loop(void) {
    /*
     * Step scheduler: check all channels, generate steps at correct times.
     * This runs in a tight loop on Core 1.
     *
     * For each channel:
     *   1. If not active, try to dequeue a segment
     *   2. If active, check if it's time for the next step
     *   3. Collect all channels that need a step right now
     *   4. Build SR word and push to PIO
     */

    uint32_t now = time_us_32();
    bool any_step = false;
    bool step_bits[STEPPER_COUNT] = {false};

    /* Read endstop state once per loop iteration */
    uint8_t endstops = hydra_gpio_read_endstops();

    for (int ch = 0; ch < STEPPER_COUNT; ch++) {
        step_state_t *st = &step_states[ch];

        /* Check endstop during homing — halt channel if triggered */
        if (homing_endstop_bit[ch] && (endstops & homing_endstop_bit[ch])) {
            if (st->active) {
                st->active = false;
                st->steps_remaining = 0;
            }
            /* Flush remaining queue entries */
            channels[ch].tail = channels[ch].head;
            homing_triggered[ch] = true;
            continue;
        }

        /* Load next segment if idle */
        if (!st->active) {
            stepper_channel_t *chan = &channels[ch];
            if (chan->tail != chan->head) {
                step_segment_t *seg = &chan->buf[chan->tail];
                st->interval = seg->interval;
                st->add = seg->add;
                st->steps_remaining = seg->count;
                st->next_step_time = now + st->interval;
                st->active = true;
                __dmb();
                chan->tail = (chan->tail + 1) % STEP_QUEUE_SIZE;
            }
        }

        /* Generate step if it's time */
        if (st->active && (int32_t)(now - st->next_step_time) >= 0) {
            step_bits[ch] = true;
            any_step = true;

            /* Update position */
            if (channels[ch].dir)
                channels[ch].position--;
            else
                channels[ch].position++;

            st->steps_remaining--;
            if (st->steps_remaining == 0) {
                st->active = false;
            } else {
                /* Apply linear ramp: interval += add */
                st->interval = (uint32_t)((int32_t)st->interval + st->add);
                if (st->interval < 10) st->interval = 10; /* Safety floor */
                st->next_step_time = now + st->interval;
            }
        }
    }

    if (any_step) {
        uint16_t word = build_sr_word(step_bits);
        /* Push step word to PIO (step pulse) */
        pio_sm_put_blocking(step_pio, step_sm, word);

        /* Brief delay then clear step bits (direction stays) */
        busy_wait_us_32(2);

        bool no_steps[STEPPER_COUNT] = {false};
        uint16_t clear_word = build_sr_word(no_steps);
        pio_sm_put_blocking(step_pio, step_sm, clear_word);
    } else {
        tight_loop_contents();
    }
}

bool stepper_queue(uint8_t channel, const hydra_queue_step_t *cmd) {
    if (channel >= STEPPER_COUNT) return false;

    stepper_channel_t *ch = &channels[channel];
    uint16_t next_head = (ch->head + 1) % STEP_QUEUE_SIZE;

    if (next_head == ch->tail) return false;

    ch->buf[ch->head].interval = cmd->interval;
    ch->buf[ch->head].count = cmd->count;
    ch->buf[ch->head].add = cmd->add;
    __dmb();
    ch->head = next_head;

    return true;
}

void stepper_set_dir(uint8_t channel, uint8_t dir) {
    if (channel < STEPPER_COUNT) {
        channels[channel].dir = dir ? 1 : 0;
    }
}

void stepper_reset_clock(uint8_t channel, uint32_t clock) {
    if (channel >= STEPPER_COUNT) return;
    step_state_t *st = &step_states[channel];
    st->next_step_time = clock;
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
    return (uint8_t)((depth * 255) / STEP_QUEUE_SIZE);
}

void stepper_emergency_stop(void) {
    for (int i = 0; i < STEPPER_COUNT; i++) {
        channels[i].head = 0;
        channels[i].tail = 0;
        step_states[i].active = false;
    }
    stepper_enable(false);
}

void stepper_enable(bool enable) {
    gpio_put(PIN_STEPPER_EN, enable ? 0 : 1);
}

void stepper_set_homing(uint8_t channel, uint8_t endstop_bit) {
    if (channel >= STEPPER_COUNT) return;
    homing_endstop_bit[channel] = endstop_bit;
    homing_triggered[channel] = false;
}

bool stepper_homing_triggered(uint8_t channel) {
    if (channel >= STEPPER_COUNT) return false;
    bool triggered = homing_triggered[channel];
    if (triggered) {
        homing_triggered[channel] = false;
    }
    return triggered;
}

void stepper_flush_channel(uint8_t channel) {
    if (channel >= STEPPER_COUNT) return;
    channels[channel].head = 0;
    channels[channel].tail = 0;
    step_states[channel].active = false;
}
