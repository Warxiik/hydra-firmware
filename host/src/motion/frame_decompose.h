#pragma once

#include "planner.h"
#include "nozzle_offset.h"
#include "collision.h"
#include "corexy.h"
#include "../core/config.h"
#include <optional>
#include <array>

namespace hydra::motion {

/**
 * Motor-level move output from frame decomposition.
 * Contains step counts and speeds for all 7 motor channels.
 */
struct DecomposedMove {
    struct AxisMove {
        int32_t steps = 0;       /* Signed: negative = reverse direction */
        double speed_steps_s = 0; /* Step rate (steps/second) */
    };

    AxisMove corexy_a;    /* Channel 0: CoreXY A = frame X + frame Y */
    AxisMove corexy_b;    /* Channel 1: CoreXY B = frame X - frame Y */
    AxisMove offset_x;    /* Channel 2: Nozzle 1 relative X */
    AxisMove offset_y;    /* Channel 3: Nozzle 1 relative Y */
    AxisMove z;           /* Channel 4: Bed Z */
    AxisMove extruder_0;  /* Channel 5: Nozzle 0 extruder */
    AxisMove extruder_1;  /* Channel 6: Nozzle 1 extruder */

    double duration_s = 0; /* Move duration for synchronization */
};

/**
 * Frame motion decomposer for dual-nozzle printing.
 *
 * When both nozzles are active, converts two nozzle-space moves into
 * motor-space commands:
 *   - Frame (CoreXY A,B) follows nozzle 0
 *   - Offset actuators (X,Y) position nozzle 1 relative to frame
 *   - Each extruder runs independently
 *
 * When only one nozzle is active, the other stays parked (no offset change).
 */
class FrameDecomposer {
public:
    explicit FrameDecomposer(const Config& config);

    /**
     * Decompose a move for a single nozzle.
     * Updates internal nozzle position tracking.
     *
     * @param nozzle 0 or 1
     * @param move   Planned move from the per-nozzle planner
     * @return Motor-level move, or nullopt if collision detected
     */
    std::optional<DecomposedMove> decompose(int nozzle, const PlannedMove& move);

    /**
     * Get current tracked position for a nozzle.
     */
    CartesianPos nozzle_position(int nozzle) const { return nozzle_pos_[nozzle]; }

    /**
     * Get current frame position.
     */
    CartesianPos frame_position() const { return frame_pos_; }

    /**
     * Reset all tracked positions to zero.
     */
    void reset();

private:
    const Config& config_;
    CollisionValidator collision_;

    /* Current tracked positions */
    CartesianPos nozzle_pos_[2];  /* Nozzle positions in Cartesian space */
    CartesianPos frame_pos_;      /* Frame (= nozzle 0) position */
    double offset_x_ = 0;        /* Current offset actuator X */
    double offset_y_ = 0;        /* Current offset actuator Y */
};

} // namespace hydra::motion
