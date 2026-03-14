#pragma once

#include "transport.h"
#include "protocol_defs.h"
#include <cstdint>
#include <optional>
#include <vector>

namespace hydra::comms {

/**
 * High-level protocol encoder/decoder.
 * Builds command payloads and parses response payloads.
 */
class Protocol {
public:
    explicit Protocol(Transport& transport);

    /* Basic commands */
    bool send_nop();
    std::optional<uint32_t> get_clock();
    bool emergency_stop();

    /* Stepper commands */
    bool queue_step(uint8_t channel, uint32_t interval, uint16_t count, int16_t add);
    bool set_step_dir(uint8_t channel, uint8_t dir);
    bool reset_step_clock(uint8_t channel, uint32_t clock);
    std::optional<int32_t> get_stepper_position(uint8_t channel);

    /* PWM */
    bool set_pwm(uint8_t channel, uint16_t duty);

    /* Endstop / Homing */
    bool endstop_home(uint8_t channel, uint8_t endstop_bit);
    struct EndstopQueryResult {
        uint8_t endstop_state;
        uint8_t trigger_flags; /* Bit N = channel N homing triggered */
    };
    std::optional<EndstopQueryResult> endstop_query();

    /* TMC2209 */
    bool tmc_write(uint8_t driver, uint8_t reg, uint32_t value);
    std::optional<uint32_t> tmc_read(uint8_t driver, uint8_t reg);

    /* Status report (parsed from RSP_STATUS) */
    struct StatusReport {
        uint32_t mcu_clock;
        uint16_t adc_raw[ADC_CHANNEL_COUNT];
        uint8_t endstop_state;
        uint8_t queue_depth[STEPPER_COUNT];
        uint8_t flags;
    };

    std::optional<StatusReport> get_status();

private:
    /** Send command and expect a specific response type */
    std::optional<std::vector<uint8_t>> command(const uint8_t* cmd, uint16_t len);

    Transport& transport_;
};

} // namespace hydra::comms
