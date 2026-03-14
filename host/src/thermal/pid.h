#pragma once

namespace hydra::thermal {

/**
 * PID controller with anti-windup clamping.
 * Output is 0.0–1.0 (duty cycle for heater PWM).
 */
class PidController {
public:
    struct Gains {
        double kp = 20.0;
        double ki = 1.0;
        double kd = 100.0;
    };

    explicit PidController(const Gains& gains);

    /** Update the controller with a new measurement. Returns output 0.0–1.0. */
    double update(double setpoint, double measurement, double dt_s);

    /** Reset integrator and derivative state */
    void reset();

    /** Update gains live (e.g. after auto-tune) */
    void set_gains(const Gains& gains) { gains_ = gains; }
    const Gains& gains() const { return gains_; }

private:
    Gains gains_;
    double integral_ = 0.0;
    double prev_error_ = 0.0;
    bool first_update_ = true;
};

} // namespace hydra::thermal
