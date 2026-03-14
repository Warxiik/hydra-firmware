#include "safety.h"
#include <spdlog/spdlog.h>

namespace hydra::thermal {

SafetyMonitor::SafetyMonitor() : config_{} {}
SafetyMonitor::SafetyMonitor(const Config& config) : config_(config) {}

bool SafetyMonitor::update(double current_temp, double target_temp, double pwm_output,
                           double dt_s, std::string& reason) {
    /* Check absolute temperature limits */
    if (current_temp > config_.max_temp) {
        reason = "Temperature exceeds maximum (" + std::to_string(config_.max_temp) + "°C)";
        return false;
    }

    if (current_temp < config_.min_temp) {
        reason = "Temperature below minimum — sensor fault or disconnected";
        return false;
    }

    /* Thermal runaway detection:
     * If we're applying significant power (>50% PWM) and temp isn't rising,
     * something is wrong (heater disconnected, thermistor fallen off, etc.) */
    bool heating = target_temp > 0 && pwm_output > 0.5;

    if (heating) {
        if (!was_heating_) {
            /* Just started heating — record baseline */
            baseline_temp_ = current_temp;
            heating_timer_ = 0.0;
            was_heating_ = true;
        }

        heating_timer_ += dt_s;

        /* Check if temp has risen enough since we started */
        double rise = current_temp - baseline_temp_;

        if (heating_timer_ > config_.runaway_timeout_s && rise < config_.runaway_min_rise) {
            reason = "Thermal runaway: no temperature rise after "
                   + std::to_string(static_cast<int>(config_.runaway_timeout_s)) + "s at full power";
            return false;
        }

        /* Reset baseline periodically to track ongoing rise */
        if (heating_timer_ > config_.runaway_timeout_s * 0.5 && rise >= config_.runaway_min_rise) {
            baseline_temp_ = current_temp;
            heating_timer_ = 0.0;
        }
    } else {
        was_heating_ = false;
        heating_timer_ = 0.0;
    }

    return true;
}

void SafetyMonitor::reset() {
    heating_timer_ = 0.0;
    baseline_temp_ = 0.0;
    was_heating_ = false;
}

} // namespace hydra::thermal
