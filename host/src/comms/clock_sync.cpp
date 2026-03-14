#include "clock_sync.h"
#include <spdlog/spdlog.h>
#include <cmath>

namespace hydra::comms {

ClockSync::ClockSync(Protocol& protocol)
    : protocol_(protocol), epoch_(std::chrono::steady_clock::now()) {}

static int64_t host_us_since(std::chrono::steady_clock::time_point epoch) {
    auto now = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::microseconds>(now - epoch).count();
}

bool ClockSync::sync_once() {
    int64_t t1 = host_us_since(epoch_);
    auto mcu_clock = protocol_.get_clock();
    int64_t t2 = host_us_since(epoch_);

    if (!mcu_clock) return false;

    uint32_t rtt = static_cast<uint32_t>(t2 - t1);
    int64_t host_mid = (t1 + t2) / 2;

    /* Store sample in ring buffer */
    int idx = sample_idx_ % MAX_SAMPLES;
    samples_[idx] = {host_mid, *mcu_clock, rtt};
    sample_idx_++;
    if (sample_count_ < MAX_SAMPLES) sample_count_++;

    rtt_us_ = rtt;

    /* Update linear regression with accumulated samples */
    if (sample_count_ >= MIN_SAMPLES) {
        update_regression();
    } else {
        /* Simple offset estimate until we have enough samples */
        offset_ = static_cast<double>(*mcu_clock) - static_cast<double>(host_mid);
        drift_rate_ = 1.0;
    }

    return true;
}

void ClockSync::update_regression() {
    /*
     * Least-squares linear regression: mcu_us = offset + drift_rate * host_us
     *
     * We weight samples by inverse RTT — lower RTT = more precise estimate.
     */
    double sum_w = 0, sum_wx = 0, sum_wy = 0, sum_wxx = 0, sum_wxy = 0;

    int n = sample_count_;
    int start = (sample_idx_ >= MAX_SAMPLES) ? sample_idx_ % MAX_SAMPLES : 0;

    for (int i = 0; i < n; i++) {
        int idx = (start + i) % MAX_SAMPLES;
        const auto& s = samples_[idx];

        double w = 1.0 / std::max(s.rtt_us, 1u); /* Weight by inverse RTT */
        double x = static_cast<double>(s.host_us);
        double y = static_cast<double>(s.mcu_us);

        sum_w += w;
        sum_wx += w * x;
        sum_wy += w * y;
        sum_wxx += w * x * x;
        sum_wxy += w * x * y;
    }

    double denom = sum_w * sum_wxx - sum_wx * sum_wx;
    if (std::abs(denom) < 1e-10) return; /* Degenerate case */

    drift_rate_ = (sum_w * sum_wxy - sum_wx * sum_wy) / denom;
    offset_ = (sum_wy - drift_rate_ * sum_wx) / sum_w;

    /* Sanity check: drift should be close to 1.0 (both clocks are in µs) */
    if (drift_rate_ < 0.99 || drift_rate_ > 1.01) {
        spdlog::warn("Clock drift outside expected range: {:.6f}", drift_rate_);
    }
}

uint32_t ClockSync::host_to_mcu(std::chrono::steady_clock::time_point t) const {
    auto us = std::chrono::duration_cast<std::chrono::microseconds>(t - epoch_).count();
    double mcu = offset_ + drift_rate_ * static_cast<double>(us);
    return static_cast<uint32_t>(mcu);
}

std::chrono::steady_clock::time_point ClockSync::mcu_to_host(uint32_t mcu_clock) const {
    double host_us = (static_cast<double>(mcu_clock) - offset_) / drift_rate_;
    return epoch_ + std::chrono::microseconds(static_cast<int64_t>(host_us));
}

uint32_t ClockSync::estimated_mcu_clock() const {
    return host_to_mcu(std::chrono::steady_clock::now());
}

} // namespace hydra::comms
