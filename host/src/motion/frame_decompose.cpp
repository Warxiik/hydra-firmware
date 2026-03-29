#include "frame_decompose.h"
#include <cmath>

namespace hydra::motion {

FrameDecomposer::FrameDecomposer(const Config& config)
    : config_(config) {}

void FrameDecomposer::reset() {
    pos_ = {};
}

DecomposedMove FrameDecomposer::decompose(const PlannedMove& move) {
    DecomposedMove result;
    result.valve_mask = move.valve_mask;

    /* Compute Cartesian deltas */
    double dx = move.end.x - move.start.x;
    double dy = move.end.y - move.start.y;
    double dz = move.end.z - move.start.z;
    double de = move.end.e - move.start.e;

    /* Compute duration */
    double duration = 0;
    if (move.cruise_speed > 0 && move.distance > 0) {
        duration = move.distance / move.cruise_speed;
    }
    result.duration_s = duration;

    /* Convert XY to CoreXY motor space */
    auto motor_start = cartesian_to_corexy(move.start.x, move.start.y);
    auto motor_end = cartesian_to_corexy(move.end.x, move.end.y);
    double da = motor_end.a - motor_start.a;
    double db = motor_end.b - motor_start.b;

    /* Helper: compute steps and speed for an axis */
    auto compute_axis = [&](double delta_mm, double steps_per_mm) -> DecomposedMove::AxisMove {
        DecomposedMove::AxisMove axis;
        axis.steps = static_cast<int32_t>(std::round(delta_mm * steps_per_mm));
        if (duration > 0 && axis.steps != 0) {
            axis.speed_steps_s = std::abs(delta_mm * steps_per_mm) / duration;
        }
        return axis;
    };

    result.corexy_a = compute_axis(da, config_.steps_per_mm_xy);
    result.corexy_b = compute_axis(db, config_.steps_per_mm_xy);

    /* Z: all three lead screws move together */
    auto z_axis = compute_axis(dz, config_.steps_per_mm_z);
    result.z1 = z_axis;
    result.z2 = z_axis;
    result.z3 = z_axis;

    /* Extruder: scale speed by number of open nozzles */
    int open_nozzles = __builtin_popcount(move.valve_mask & 0x0F);
    double e_scale = (open_nozzles > 0) ? static_cast<double>(open_nozzles) : 1.0;
    result.extruder = compute_axis(de * e_scale, config_.steps_per_mm_e);

    /* Update tracked position */
    pos_ = move.end;

    return result;
}

} // namespace hydra::motion
