#pragma once

#include "pid.h"
#include "thermistor.h"
#include "safety.h"
#include <cstdint>
#include <string>

namespace hydra::thermal {

/**
 * Heater controller: combines thermistor reading, PID, and safety.
 * One instance per heater (manifold, bed).
 */
class Heater {
public:
    struct Config {
        std::string name;
        uint8_t pwm_channel;
        uint8_t adc_channel;
        PidController::Gains pid_gains;
        double max_temp;          /* Hard limit (°C) */
        double min_temp = -10.0;  /* Below this = sensor fault */
        Thermistor::Coefficients therm_coeff;
    };

    explicit Heater(const Config& config);

    /** Set target temperature (0 = off) */
    void set_target(double temp_c);
    double target() const { return target_; }

    /** Update with new ADC reading. Returns PWM duty (0–65535). */
    uint16_t update(uint16_t adc_raw, double dt_s);

    /** Current temperature reading */
    double current_temp() const { return current_temp_; }

    /** Is temperature within ±2°C of target? */
    bool at_target() const;

    /** Safety fault active? */
    bool has_fault() const { return fault_; }
    const std::string& fault_reason() const { return fault_reason_; }

    /** Emergency shutdown */
    void kill();

    const std::string& name() const { return config_.name; }

private:
    Config config_;
    Thermistor thermistor_;
    PidController pid_;
    SafetyMonitor safety_;

    double target_ = 0.0;
    double current_temp_ = 0.0;
    bool fault_ = false;
    std::string fault_reason_;
};

} // namespace hydra::thermal
