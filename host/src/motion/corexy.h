#pragma once

namespace hydra::motion {

/**
 * CoreXY kinematics transforms.
 *
 * CoreXY uses two motors (A and B) to achieve XY motion:
 *   A = X + Y    (motor A drives belt going diagonally one way)
 *   B = X - Y    (motor B drives belt going diagonally the other)
 *
 * Inverse: X = (A + B) / 2,  Y = (A - B) / 2
 */

struct CoreXYPos {
    double a;  /* Motor A position (steps or mm) */
    double b;  /* Motor B position (steps or mm) */
};

struct CartesianXY {
    double x;
    double y;
};

/** Cartesian XY → CoreXY motor positions (forward kinematics) */
inline CoreXYPos cartesian_to_corexy(double x, double y) {
    return {x + y, x - y};
}

/** CoreXY motor positions → Cartesian XY (inverse kinematics) */
inline CartesianXY corexy_to_cartesian(double a, double b) {
    return {(a + b) / 2.0, (a - b) / 2.0};
}

/**
 * Convert a Cartesian velocity (mm/s) to CoreXY motor velocities.
 * Same transform applies to velocities and accelerations.
 */
inline CoreXYPos cartesian_vel_to_corexy(double vx, double vy) {
    return {vx + vy, vx - vy};
}

} // namespace hydra::motion
