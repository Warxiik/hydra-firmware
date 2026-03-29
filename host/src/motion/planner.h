#pragma once

#include <cstdint>
#include <optional>

namespace hydra::motion {

struct CartesianPos {
    double x = 0, y = 0, z = 0, e = 0;
};

struct PlannedMove {
    CartesianPos start, end;
    double distance;
    double entry_speed;
    double cruise_speed;
    double exit_speed;
    double acceleration;
    bool is_travel;
    uint8_t valve_mask = 0;  /* Which nozzles are open during this move */
};

/**
 * Move planner with lookahead buffer.
 * One instance per nozzle.
 */
class Planner {
public:
    static constexpr int LOOKAHEAD_SIZE = 64;

    struct Config {
        double max_velocity = 300.0;
        double max_acceleration = 3000.0;
        double max_z_velocity = 10.0;
        double junction_deviation = 0.05;
    };

    explicit Planner(const Config& config);

    void push(const CartesianPos& target, double feedrate, bool is_travel,
              uint8_t valve_mask = 0);
    std::optional<PlannedMove> pop();
    void flush();
    void clear();
    int queue_depth() const;

private:
    void recalculate();
    double junction_speed(const PlannedMove& prev, const PlannedMove& next) const;

    Config config_;
    PlannedMove buffer_[LOOKAHEAD_SIZE];
    int head_ = 0, tail_ = 0, count_ = 0;
    CartesianPos current_pos_;
};

} // namespace hydra::motion
