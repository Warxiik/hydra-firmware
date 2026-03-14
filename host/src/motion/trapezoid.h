#pragma once

#include <cstdint>

namespace hydra::motion {

/**
 * Trapezoidal velocity profile generator.
 * Given a move distance and velocity constraints, computes
 * the three phases: accelerate, cruise, decelerate.
 */
struct TrapezoidProfile {
    double accel_distance;    /* Distance during acceleration phase (mm) */
    double cruise_distance;   /* Distance at constant velocity (mm) */
    double decel_distance;    /* Distance during deceleration phase (mm) */
    double entry_speed;       /* Speed at start of move (mm/s) */
    double cruise_speed;      /* Maximum speed during move (mm/s) */
    double exit_speed;        /* Speed at end of move (mm/s) */
    double acceleration;      /* Acceleration rate (mm/s²) */
    double total_time;        /* Total time for the move (s) */
};

/**
 * Compute a trapezoidal velocity profile for a move.
 *
 * @param distance      Total move distance (mm, always positive)
 * @param entry_speed   Starting velocity (mm/s)
 * @param exit_speed    Ending velocity (mm/s)
 * @param cruise_speed  Maximum velocity (mm/s)
 * @param acceleration  Acceleration/deceleration rate (mm/s²)
 */
TrapezoidProfile compute_trapezoid(double distance, double entry_speed,
                                   double exit_speed, double cruise_speed,
                                   double acceleration);

} // namespace hydra::motion
