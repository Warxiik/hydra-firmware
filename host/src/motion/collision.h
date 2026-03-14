#pragma once

#include "nozzle_offset.h"

namespace hydra::motion {

/**
 * Nozzle collision validator.
 *
 * Ensures the two nozzles don't collide by checking that the
 * offset actuator stays within its mechanical limits and that
 * the nozzles maintain a minimum separation.
 */
class CollisionValidator {
public:
    struct Config {
        NozzleOffsetConfig offset_limits;
        double min_nozzle_separation = 5.0;  /* mm — minimum gap between nozzles */
        double nozzle_diameter = 0.4;        /* mm */
    };

    CollisionValidator();
    explicit CollisionValidator(const Config& config);

    /**
     * Check if a dual-nozzle move is safe.
     * Returns true if no collision would occur.
     */
    bool is_safe(double n0_x, double n0_y, double n1_x, double n1_y) const;

    /**
     * Check if the offset is within actuator range.
     */
    bool offset_valid(double offset_x, double offset_y) const;

    /**
     * Compute the closest the two nozzles can get on a line segment
     * between two dual positions. Returns minimum distance.
     */
    double min_separation(double n0_x0, double n0_y0, double n1_x0, double n1_y0,
                          double n0_x1, double n0_y1, double n1_x1, double n1_y1) const;

private:
    Config config_;
};

} // namespace hydra::motion
