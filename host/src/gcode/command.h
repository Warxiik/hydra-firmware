#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <variant>

namespace hydra::gcode {

struct MoveCommand {
    std::optional<double> x, y, z, e, f;
    bool rapid = false; /* G0 vs G1 */
};

struct HomeCommand {
    bool x = true, y = true, z = true;
};

struct TempCommand {
    enum Target { Nozzle, Bed };
    Target target;
    int nozzle_id = 0;
    double temp_c;
    bool wait = false; /* M109/M190 vs M104/M140 */
};

struct FanCommand {
    int fan_id = 0;
    int speed = 0; /* 0-255, or -1 for M107 (off) */
};

struct AccelCommand {
    std::optional<double> print_accel;  /* M204 S */
    std::optional<double> travel_accel; /* M204 T */
    std::optional<double> jerk_x;      /* M205 X */
    std::optional<double> jerk_y;      /* M205 Y */
};

struct ValveSet {
    uint8_t mask = 0;  /* Bitmask: bit N = nozzle N open */
};

struct SetPosition {
    std::optional<double> x, y, z, e; /* G92 */
};

struct FilamentChange {};

struct NopCommand {};

using Command = std::variant<
    NopCommand,
    MoveCommand,
    HomeCommand,
    TempCommand,
    FanCommand,
    AccelCommand,
    ValveSet,
    SetPosition,
    FilamentChange
>;

} // namespace hydra::gcode
