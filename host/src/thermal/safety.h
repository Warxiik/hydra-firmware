#pragma once

#include <cstdint>
#include <string>

namespace hydra::thermal {

/**
 * Thermal safety monitor.
 * Detects runaway conditions and sensor faults.
 */
class SafetyMonitor {
public:
    struct Config {
        double max_temp = 300.0;          /* Absolute max temperature (°C) */
        double min_temp = -10.0;          /* Below this = sensor fault */
        double runaway_timeout_s = 45.0;  /* Max time at full power without temp rise */
        double runaway_min_rise = 2.0;    /* Minimum temp rise to not be "stuck" */
        double cooling_timeout_s = 120.0; /* Max time to cool after target set to 0 */
    };

    SafetyMonitor();
    explicit SafetyMonitor(const Config& config);

    /**
     * Update safety state. Returns true if safe, false if fault.
     * When returning false, reason is set to a human-readable fault description.
     */
    bool update(double current_temp, double target_temp, double pwm_output,
                double dt_s, std::string& reason);

    void reset();

private:
    Config config_;
    double heating_timer_ = 0.0;    /* Time at high power without temp rise */
    double baseline_temp_ = 0.0;    /* Temp when heating started */
    bool was_heating_ = false;
};

} // namespace hydra::thermal
