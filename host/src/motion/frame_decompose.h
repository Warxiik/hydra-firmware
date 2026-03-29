#pragma once

#include "planner.h"
#include "corexy.h"
#include "../core/config.h"
#include <optional>

namespace hydra::motion {

/**
 * Motor-level move output from frame decomposition.
 * Contains step counts and speeds for all motor channels.
 */
struct DecomposedMove {
    struct AxisMove {
        int32_t steps = 0;       /* Signed: negative = reverse direction */
        double speed_steps_s = 0; /* Step rate (steps/second) */
    };

    AxisMove corexy_a;    /* Channel 0: CoreXY A = X + Y */
    AxisMove corexy_b;    /* Channel 1: CoreXY B = X - Y */
    AxisMove z1;          /* Channel 2: Z lead screw 1   */
    AxisMove z2;          /* Channel 3: Z lead screw 2   */
    AxisMove z3;          /* Channel 4: Z lead screw 3   */
    AxisMove extruder;    /* Channel 5: Shared extruder   */

    uint8_t valve_mask = 0; /* Which nozzles are open for this move */
    double duration_s = 0;  /* Move duration for synchronization */
};

/**
 * Standard CoreXY frame decomposer for multi-nozzle valve array.
 *
 * Converts Cartesian moves into CoreXY motor commands.
 * No offset actuators — all nozzles are fixed on the shared carriage.
 * Valve mask selects which nozzles extrude.
 * Extruder speed is scaled by the number of open nozzles.
 */
class FrameDecomposer {
public:
    explicit FrameDecomposer(const Config& config);

    /**
     * Decompose a planned move into motor-level commands.
     */
    DecomposedMove decompose(const PlannedMove& move);

    CartesianPos current_position() const { return pos_; }
    void reset();

private:
    const Config& config_;
    CartesianPos pos_;
};

} // namespace hydra::motion
