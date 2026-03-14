#pragma once

#include "protocol.h"
#include <cstdint>
#include <chrono>

namespace hydra::comms {

/**
 * Host ↔ MCU clock synchronization.
 *
 * Uses periodic GET_CLOCK exchanges to build a linear regression
 * mapping between host monotonic time and MCU clock (µs counter).
 * This lets the host schedule steps in MCU time coordinates.
 */
class ClockSync {
public:
    explicit ClockSync(Protocol& protocol);

    /** Run a synchronization round (call periodically, e.g. every 100ms) */
    bool sync_once();

    /** Convert host time to MCU clock ticks */
    uint32_t host_to_mcu(std::chrono::steady_clock::time_point t) const;

    /** Convert MCU clock ticks to host time */
    std::chrono::steady_clock::time_point mcu_to_host(uint32_t mcu_clock) const;

    /** Get current estimated MCU clock */
    uint32_t estimated_mcu_clock() const;

    /** Clock drift rate (MCU ticks per host µs, ideally ~1.0) */
    double drift_rate() const { return drift_rate_; }

    /** Estimated round-trip time in µs */
    uint32_t rtt_us() const { return rtt_us_; }

    /** Is the sync calibrated (enough samples)? */
    bool is_calibrated() const { return sample_count_ >= MIN_SAMPLES; }

private:
    static constexpr int MAX_SAMPLES = 16;
    static constexpr int MIN_SAMPLES = 4;

    struct Sample {
        int64_t host_us;   /* Host monotonic µs */
        uint32_t mcu_us;   /* MCU clock value */
        uint32_t rtt_us;   /* Round-trip time */
    };

    void update_regression();

    Protocol& protocol_;
    Sample samples_[MAX_SAMPLES];
    int sample_count_ = 0;
    int sample_idx_ = 0;

    /* Linear model: mcu_clock = offset + drift_rate * host_us */
    double offset_ = 0;
    double drift_rate_ = 1.0;
    uint32_t rtt_us_ = 0;
    std::chrono::steady_clock::time_point epoch_;
};

} // namespace hydra::comms
