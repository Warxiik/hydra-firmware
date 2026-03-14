#include "heater.h"
#include <spdlog/spdlog.h>
#include <cmath>

namespace hydra::thermal {

Heater::Heater(const Config& config)
    : config_(config)
    , thermistor_(config.therm_coeff)
    , pid_(config.pid_gains)
    , safety_(SafetyMonitor::Config{config.max_temp, config.min_temp}) {}

void Heater::set_target(double temp_c) {
    if (temp_c < 0) temp_c = 0;
    if (temp_c > config_.max_temp) {
        spdlog::warn("Heater {}: target {:.0f}°C exceeds max {:.0f}°C, clamping",
                     config_.name, temp_c, config_.max_temp);
        temp_c = config_.max_temp;
    }

    if (target_ != temp_c) {
        spdlog::info("Heater {}: target → {:.0f}°C", config_.name, temp_c);
        target_ = temp_c;

        if (temp_c == 0.0) {
            pid_.reset();
        }
    }
}

uint16_t Heater::update(uint16_t adc_raw, double dt_s) {
    /* Convert ADC to temperature */
    current_temp_ = thermistor_.adc_to_celsius(adc_raw);

    /* Safety check */
    std::string reason;
    double pwm_float = 0.0;

    if (fault_) {
        return 0; /* Stay off once faulted */
    }

    if (!thermistor_.is_valid(adc_raw)) {
        fault_ = true;
        fault_reason_ = "Thermistor read invalid (open/short circuit)";
        spdlog::error("Heater {}: {}", config_.name, fault_reason_);
        return 0;
    }

    /* Run PID if target is set */
    if (target_ > 0.0) {
        pwm_float = pid_.update(target_, current_temp_, dt_s);
    }

    /* Safety monitor */
    if (!safety_.update(current_temp_, target_, pwm_float, dt_s, reason)) {
        fault_ = true;
        fault_reason_ = reason;
        spdlog::error("Heater {}: SAFETY FAULT — {}", config_.name, reason);
        return 0;
    }

    /* Convert 0.0–1.0 to 0–65535 PWM duty */
    return static_cast<uint16_t>(pwm_float * 65535.0);
}

bool Heater::at_target() const {
    if (target_ <= 0.0) return false;
    return std::abs(current_temp_ - target_) <= 2.0;
}

void Heater::kill() {
    target_ = 0.0;
    pid_.reset();
    safety_.reset();
}

} // namespace hydra::thermal
