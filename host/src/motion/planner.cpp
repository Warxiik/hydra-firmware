#include "planner.h"
#include "trapezoid.h"
#include <cmath>
#include <algorithm>

namespace hydra::motion {

Planner::Planner(const Config& config) : config_(config) {}

void Planner::push(const CartesianPos& target, double feedrate, bool is_travel) {
    if (count_ >= LOOKAHEAD_SIZE) return; /* Buffer full */

    PlannedMove move;
    move.start = current_pos_;
    move.end = target;
    move.is_travel = is_travel;

    /* Compute distance */
    double dx = target.x - current_pos_.x;
    double dy = target.y - current_pos_.y;
    double dz = target.z - current_pos_.z;
    double de = target.e - current_pos_.e;

    /* XY distance for feedrate purposes */
    double xy_dist = std::sqrt(dx * dx + dy * dy);
    move.distance = (xy_dist > 0.001) ? xy_dist : std::abs(dz) + std::abs(de);

    if (move.distance < 0.0001) return; /* Skip zero-length moves */

    /* Limit feedrate by axis constraints */
    double max_speed = feedrate;
    if (std::abs(dz) > 0.001) {
        double z_ratio = std::abs(dz) / move.distance;
        max_speed = std::min(max_speed, config_.max_z_velocity / z_ratio);
    }
    max_speed = std::min(max_speed, config_.max_velocity);

    move.cruise_speed = max_speed;
    move.acceleration = config_.max_acceleration;
    move.entry_speed = 0.0;  /* Will be refined by recalculate() */
    move.exit_speed = 0.0;

    /* Insert into buffer */
    int idx = (head_ + count_) % LOOKAHEAD_SIZE;
    buffer_[idx] = move;
    count_++;

    current_pos_ = target;
    recalculate();
}

std::optional<PlannedMove> Planner::pop() {
    if (count_ <= 1) return std::nullopt; /* Keep at least one for lookahead */

    PlannedMove move = buffer_[head_];
    head_ = (head_ + 1) % LOOKAHEAD_SIZE;
    count_--;
    return move;
}

void Planner::flush() {
    /* Allow draining all moves including the last one */
    while (count_ > 0) {
        /* Set last move exit speed to 0 */
        int last_idx = (head_ + count_ - 1) % LOOKAHEAD_SIZE;
        buffer_[last_idx].exit_speed = 0.0;
    }
}

void Planner::clear() {
    head_ = 0;
    count_ = 0;
}

int Planner::queue_depth() const {
    return count_;
}

double Planner::junction_speed(const PlannedMove& prev, const PlannedMove& next) const {
    /*
     * Junction deviation model (from Marlin/Klipper):
     * Compute the maximum speed at the junction between two moves
     * based on the angle between them and the configured junction deviation.
     *
     * v_junction = sqrt(junction_deviation * acceleration *
     *              sin(angle/2) / (1 - sin(angle/2)))
     */
    if (prev.distance < 0.0001 || next.distance < 0.0001)
        return 0.0;

    /* Direction vectors */
    double px = (prev.end.x - prev.start.x) / prev.distance;
    double py = (prev.end.y - prev.start.y) / prev.distance;
    double nx = (next.end.x - next.start.x) / next.distance;
    double ny = (next.end.y - next.start.y) / next.distance;

    /* Dot product = cos(angle) */
    double cos_angle = px * nx + py * ny;
    cos_angle = std::clamp(cos_angle, -1.0, 1.0);

    /* At 180° (reversal), junction speed = 0 */
    if (cos_angle < -0.999) return 0.0;
    /* At 0° (straight line), allow full speed */
    if (cos_angle > 0.999) return std::min(prev.cruise_speed, next.cruise_speed);

    /* sin(angle/2) = sqrt((1 - cos_angle) / 2) */
    double sin_half = std::sqrt((1.0 - cos_angle) / 2.0);

    double jd = config_.junction_deviation;
    double accel = config_.max_acceleration;

    double v_junction = std::sqrt(jd * accel * sin_half / (1.0 - sin_half));

    /* Clamp to move speeds */
    v_junction = std::min(v_junction, prev.cruise_speed);
    v_junction = std::min(v_junction, next.cruise_speed);

    return v_junction;
}

void Planner::recalculate() {
    if (count_ < 2) return;

    /*
     * Reverse pass: compute maximum entry speeds based on exit constraints.
     * Forward pass: compute actual entry speeds considering acceleration limits.
     */

    /* Set last move exit to 0 (will decelerate to stop) */
    int last = (head_ + count_ - 1) % LOOKAHEAD_SIZE;
    buffer_[last].exit_speed = 0.0;

    /* Reverse pass: propagate exit speeds backwards */
    for (int i = count_ - 1; i > 0; i--) {
        int curr_idx = (head_ + i) % LOOKAHEAD_SIZE;
        int prev_idx = (head_ + i - 1) % LOOKAHEAD_SIZE;

        auto& curr = buffer_[curr_idx];
        auto& prev = buffer_[prev_idx];

        /* Maximum entry speed from junction geometry */
        double v_junc = junction_speed(prev, curr);

        /* Maximum entry speed from deceleration constraint:
         * v_entry² = v_exit² + 2 * accel * distance */
        double v_from_exit = std::sqrt(
            curr.exit_speed * curr.exit_speed + 2.0 * curr.acceleration * curr.distance);

        curr.entry_speed = std::min({v_junc, v_from_exit, curr.cruise_speed});
        prev.exit_speed = curr.entry_speed;
    }

    /* Forward pass: limit entry speeds by what we can accelerate to */
    for (int i = 1; i < count_; i++) {
        int curr_idx = (head_ + i) % LOOKAHEAD_SIZE;
        int prev_idx = (head_ + i - 1) % LOOKAHEAD_SIZE;

        auto& curr = buffer_[curr_idx];
        auto& prev = buffer_[prev_idx];

        /* Maximum speed reachable from previous move's entry + acceleration */
        double v_reachable = std::sqrt(
            prev.entry_speed * prev.entry_speed + 2.0 * prev.acceleration * prev.distance);

        if (v_reachable < curr.entry_speed) {
            curr.entry_speed = v_reachable;
            prev.exit_speed = curr.entry_speed;
        }
    }
}

} // namespace hydra::motion
