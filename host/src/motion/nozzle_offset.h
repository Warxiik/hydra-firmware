#pragma once

namespace hydra::motion {

/**
 * Nozzle offset kinematics for the dual-nozzle system.
 *
 * The Hydra has a shared CoreXY frame carrying nozzle 0 (fixed)
 * and nozzle 1 on a 2-axis offset actuator. The offset actuator
 * moves nozzle 1 relative to the frame (and nozzle 0).
 *
 * Frame position = nozzle 0 position
 * Nozzle 1 position = frame position + offset
 *
 * When both nozzles are printing, the frame must be positioned so
 * that BOTH nozzle positions are achievable within the offset range.
 */

struct NozzleOffsetConfig {
    double range_x = 30.0;  /* Max offset in X (mm from center, ± range) */
    double range_y = 30.0;  /* Max offset in Y (mm from center, ± range) */
};

struct DualPosition {
    double frame_x, frame_y;     /* CoreXY frame position */
    double offset_x, offset_y;   /* Nozzle 1 offset from frame */
};

/**
 * Compute frame + offset positions from two nozzle targets.
 *
 * Strategy: frame follows nozzle 0, offset is the difference.
 * This is the simplest approach and works when offsets are within range.
 */
inline DualPosition compute_dual_position(double n0_x, double n0_y,
                                          double n1_x, double n1_y) {
    DualPosition pos;
    pos.frame_x = n0_x;
    pos.frame_y = n0_y;
    pos.offset_x = n1_x - n0_x;
    pos.offset_y = n1_y - n0_y;
    return pos;
}

/**
 * Check if a nozzle offset is within actuator range.
 */
inline bool offset_in_range(double offset_x, double offset_y,
                            const NozzleOffsetConfig& config) {
    return offset_x >= -config.range_x && offset_x <= config.range_x
        && offset_y >= -config.range_y && offset_y <= config.range_y;
}

} // namespace hydra::motion
