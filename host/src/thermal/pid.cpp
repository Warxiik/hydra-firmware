#include "pid.h"
#include <algorithm>
#include <cmath>

namespace hydra::thermal {

PidController::PidController(const Gains& gains) : gains_(gains) {}

double PidController::update(double setpoint, double measurement, double dt_s) {
    if (dt_s <= 0.0) return 0.0;

    double error = setpoint - measurement;

    /* Proportional */
    double p = gains_.kp * error;

    /* Integral with anti-windup: only integrate when output isn't saturated */
    integral_ += error * dt_s;
    /* Clamp integral to prevent windup */
    double max_integral = 1.0 / std::max(gains_.ki, 0.001);
    integral_ = std::clamp(integral_, -max_integral, max_integral);
    double i = gains_.ki * integral_;

    /* Derivative (on measurement to avoid setpoint kick) */
    double d = 0.0;
    if (!first_update_) {
        double d_error = (error - prev_error_) / dt_s;
        d = gains_.kd * d_error;
    }
    first_update_ = false;
    prev_error_ = error;

    /* Sum and clamp to 0.0–1.0 */
    double output = p + i + d;
    return std::clamp(output, 0.0, 1.0);
}

void PidController::reset() {
    integral_ = 0.0;
    prev_error_ = 0.0;
    first_update_ = true;
}

} // namespace hydra::thermal
