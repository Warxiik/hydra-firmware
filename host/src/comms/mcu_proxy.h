#pragma once

#include "transport.h"
#include "protocol.h"
#include "clock_sync.h"
#include "../core/config.h"
#include "protocol_defs.h"
#include <memory>
#include <atomic>

namespace hydra::comms {

/**
 * High-level MCU proxy — the host's interface to all MCU hardware.
 * Owns the transport, protocol, and clock sync layers.
 */
class McuProxy {
public:
    explicit McuProxy(const Config& config);
    ~McuProxy();

    bool connect();
    void disconnect();
    bool is_connected() const { return connected_; }

    /* Clock sync (call periodically) */
    bool sync_clock();
    bool is_clock_synced() const;
    uint32_t estimated_mcu_clock() const;

    /* Stepper control */
    bool queue_step(uint8_t channel, uint32_t interval, uint16_t count, int16_t add);
    bool set_step_dir(uint8_t channel, uint8_t dir);
    bool reset_step_clock(uint8_t channel, uint32_t clock);

    /* Thermal */
    bool set_heater_pwm(uint8_t channel, uint16_t duty);
    bool set_fan_pwm(uint8_t channel, uint16_t duty);

    /* Status */
    struct Status {
        uint32_t mcu_clock = 0;
        uint16_t adc_raw[ADC_CHANNEL_COUNT] = {};
        uint8_t endstop_state = 0;
        uint8_t queue_depth[STEPPER_COUNT] = {};
        uint8_t flags = 0;
    };

    bool poll_status();
    const Status& last_status() const { return status_; }

    /* Safety */
    bool emergency_stop();
    bool heartbeat();

    /* TMC2209 */
    bool tmc_write(uint8_t driver, uint8_t reg, uint32_t value);

    /* Queue fullness check */
    bool queue_has_space(uint8_t channel, uint8_t min_free = 64) const;

private:
    std::unique_ptr<Transport> transport_;
    std::unique_ptr<Protocol> protocol_;
    std::unique_ptr<ClockSync> clock_sync_;
    Status status_;
    std::atomic<bool> connected_{false};
};

} // namespace hydra::comms

/* Convenience alias used by engine.h */
namespace hydra {
    using McuProxy = comms::McuProxy;
}
