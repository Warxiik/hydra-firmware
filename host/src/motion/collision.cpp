#include "collision.h"
#include <cmath>
#include <algorithm>

namespace hydra::motion {

CollisionValidator::CollisionValidator() : config_{} {}
CollisionValidator::CollisionValidator(const Config& config) : config_(config) {}

bool CollisionValidator::is_safe(double n0_x, double n0_y,
                                 double n1_x, double n1_y) const {
    /* Check offset range */
    double ox = n1_x - n0_x;
    double oy = n1_y - n0_y;
    if (!offset_valid(ox, oy)) return false;

    /* Check minimum separation */
    double dist = std::sqrt(ox * ox + oy * oy);
    return dist >= config_.min_nozzle_separation;
}

bool CollisionValidator::offset_valid(double offset_x, double offset_y) const {
    return offset_in_range(offset_x, offset_y, config_.offset_limits);
}

double CollisionValidator::min_separation(
    double n0_x0, double n0_y0, double n1_x0, double n1_y0,
    double n0_x1, double n0_y1, double n1_x1, double n1_y1) const {
    /*
     * Parametric check: sample along the move at several points
     * to find the minimum distance between the two nozzles.
     * t=0 is start, t=1 is end.
     */
    constexpr int SAMPLES = 10;
    double min_dist = 1e9;

    for (int i = 0; i <= SAMPLES; i++) {
        double t = static_cast<double>(i) / SAMPLES;

        double n0x = n0_x0 + t * (n0_x1 - n0_x0);
        double n0y = n0_y0 + t * (n0_y1 - n0_y0);
        double n1x = n1_x0 + t * (n1_x1 - n1_x0);
        double n1y = n1_y0 + t * (n1_y1 - n1_y0);

        double dx = n1x - n0x;
        double dy = n1y - n0y;
        double dist = std::sqrt(dx * dx + dy * dy);
        min_dist = std::min(min_dist, dist);
    }

    return min_dist;
}

} // namespace hydra::motion
