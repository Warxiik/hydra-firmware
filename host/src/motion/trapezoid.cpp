#include "trapezoid.h"
#include <cmath>
#include <algorithm>

namespace hydra::motion {

TrapezoidProfile compute_trapezoid(double distance, double entry_speed,
                                   double exit_speed, double cruise_speed,
                                   double acceleration) {
    TrapezoidProfile p{};
    p.entry_speed = entry_speed;
    p.exit_speed = exit_speed;
    p.acceleration = acceleration;

    if (distance <= 0.0 || acceleration <= 0.0) {
        p.total_time = 0.0;
        return p;
    }

    /* Clamp speeds */
    entry_speed = std::max(entry_speed, 0.0);
    exit_speed = std::max(exit_speed, 0.0);
    cruise_speed = std::max(cruise_speed, std::max(entry_speed, exit_speed));

    /*
     * v² = v0² + 2*a*d  →  d = (v² - v0²) / (2*a)
     *
     * Acceleration distance: distance to go from entry_speed to cruise_speed
     * Deceleration distance: distance to go from cruise_speed to exit_speed
     */
    double accel_d = (cruise_speed * cruise_speed - entry_speed * entry_speed) / (2.0 * acceleration);
    double decel_d = (cruise_speed * cruise_speed - exit_speed * exit_speed) / (2.0 * acceleration);

    /* Check if we can reach cruise speed */
    if (accel_d + decel_d > distance) {
        /* Triangle profile: can't reach cruise speed.
         * Find the peak speed we can actually reach. */
        double peak_sq = (2.0 * acceleration * distance
                        + entry_speed * entry_speed
                        + exit_speed * exit_speed) / 2.0;
        double peak = std::sqrt(std::max(peak_sq, 0.0));
        cruise_speed = peak;

        accel_d = (peak * peak - entry_speed * entry_speed) / (2.0 * acceleration);
        decel_d = (peak * peak - exit_speed * exit_speed) / (2.0 * acceleration);

        /* Clamp to avoid negative distances from floating-point issues */
        accel_d = std::max(accel_d, 0.0);
        decel_d = std::max(decel_d, 0.0);
    }

    double cruise_d = distance - accel_d - decel_d;
    if (cruise_d < 0.0) cruise_d = 0.0;

    p.accel_distance = accel_d;
    p.cruise_distance = cruise_d;
    p.decel_distance = decel_d;
    p.cruise_speed = cruise_speed;
    p.entry_speed = entry_speed;
    p.exit_speed = exit_speed;

    /* Compute total time */
    double t_accel = (acceleration > 0) ? (cruise_speed - entry_speed) / acceleration : 0.0;
    double t_cruise = (cruise_speed > 0) ? cruise_d / cruise_speed : 0.0;
    double t_decel = (acceleration > 0) ? (cruise_speed - exit_speed) / acceleration : 0.0;
    p.total_time = t_accel + t_cruise + t_decel;

    return p;
}

} // namespace hydra::motion
