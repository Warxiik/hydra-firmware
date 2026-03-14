#include "frame_decompose.h"
#include <spdlog/spdlog.h>
#include <cmath>

namespace hydra::motion {

FrameDecomposer::FrameDecomposer(const Config& config)
    : config_(config)
{
    CollisionValidator::Config col_cfg;
    col_cfg.offset_limits.range_x = config.offset_range_x;
    col_cfg.offset_limits.range_y = config.offset_range_y;
    collision_ = CollisionValidator(col_cfg);
}

void FrameDecomposer::reset() {
    nozzle_pos_[0] = {};
    nozzle_pos_[1] = {};
    frame_pos_ = {};
    offset_x_ = 0;
    offset_y_ = 0;
}

std::optional<DecomposedMove> FrameDecomposer::decompose(int nozzle, const PlannedMove& move) {
    if (nozzle < 0 || nozzle > 1) return std::nullopt;

    /* New nozzle position after this move */
    CartesianPos new_nozzle = move.end;

    /* The other nozzle stays at its current position */
    int other = 1 - nozzle;
    CartesianPos other_pos = nozzle_pos_[other];

    /*
     * Frame positioning strategy:
     *   Frame always follows nozzle 0.
     *   Nozzle 1 position = frame + offset.
     *
     * If nozzle 0 moves: frame moves with it, offset adjusts to keep nozzle 1 in place.
     * If nozzle 1 moves: frame stays (nozzle 0 is stationary), offset adjusts.
     */
    CartesianPos new_frame;
    double new_offset_x, new_offset_y;

    if (nozzle == 0) {
        /* Nozzle 0 is moving — frame follows */
        new_frame.x = new_nozzle.x;
        new_frame.y = new_nozzle.y;
        new_frame.z = new_nozzle.z;

        /* Offset must adjust to keep nozzle 1 stationary */
        new_offset_x = other_pos.x - new_frame.x;
        new_offset_y = other_pos.y - new_frame.y;
    } else {
        /* Nozzle 1 is moving — frame stays at nozzle 0's position */
        new_frame = frame_pos_;

        /* Offset changes to reach nozzle 1's new position */
        new_offset_x = new_nozzle.x - new_frame.x;
        new_offset_y = new_nozzle.y - new_frame.y;
    }

    /* Validate offset is within actuator range */
    if (!collision_.offset_valid(new_offset_x, new_offset_y)) {
        spdlog::error("Frame decompose: offset ({:.1f}, {:.1f}) out of range for nozzle {} move",
                      new_offset_x, new_offset_y, nozzle);
        return std::nullopt;
    }

    /* Validate collision safety */
    double n0_x = (nozzle == 0) ? new_nozzle.x : other_pos.x;
    double n0_y = (nozzle == 0) ? new_nozzle.y : other_pos.y;
    double n1_x = (nozzle == 1) ? new_nozzle.x : other_pos.x;
    double n1_y = (nozzle == 1) ? new_nozzle.y : other_pos.y;

    if (!collision_.is_safe(n0_x, n0_y, n1_x, n1_y)) {
        spdlog::error("Frame decompose: collision detected for nozzle {} move", nozzle);
        return std::nullopt;
    }

    /* Compute motor deltas */
    double frame_dx = new_frame.x - frame_pos_.x;
    double frame_dy = new_frame.y - frame_pos_.y;
    double frame_dz = new_frame.z - frame_pos_.z;
    double offset_dx = new_offset_x - offset_x_;
    double offset_dy = new_offset_y - offset_y_;
    double de = move.end.e - move.start.e;

    /* Convert frame XY to CoreXY motor space */
    auto motor_start = cartesian_to_corexy(frame_pos_.x, frame_pos_.y);
    auto motor_end = cartesian_to_corexy(new_frame.x, new_frame.y);
    double da = motor_end.a - motor_start.a;
    double db = motor_end.b - motor_start.b;

    /* Build the decomposed move */
    DecomposedMove result;

    /* Compute duration from the planner's velocity profile */
    double duration = 0;
    if (move.cruise_speed > 0 && move.distance > 0) {
        /* Simplified: use distance / cruise_speed as duration estimate */
        duration = move.distance / move.cruise_speed;
    }
    result.duration_s = duration;

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
    result.offset_x = compute_axis(offset_dx, config_.steps_per_mm_offset);
    result.offset_y = compute_axis(offset_dy, config_.steps_per_mm_offset);
    result.z = compute_axis(frame_dz, config_.steps_per_mm_z);

    /* Extruder: only the active nozzle extrudes */
    if (nozzle == 0) {
        result.extruder_0 = compute_axis(de, config_.steps_per_mm_e);
    } else {
        result.extruder_1 = compute_axis(de, config_.steps_per_mm_e);
    }

    /* Update tracked state */
    nozzle_pos_[nozzle] = new_nozzle;
    frame_pos_ = new_frame;
    offset_x_ = new_offset_x;
    offset_y_ = new_offset_y;

    return result;
}

} // namespace hydra::motion
