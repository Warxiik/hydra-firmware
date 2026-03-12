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
    int nozzle_id = 0; /* For multi-nozzle */
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

struct SyncBarrier {
    std::string id;
};

struct TaskBegin {
    uint32_t task_id;
    uint8_t nozzle;
    uint32_t layer;
};

struct TaskEnd {
    uint32_t task_id;
};

struct WaitTask {
    uint32_t task_id;
    uint8_t nozzle;
    uint32_t layer;
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
    SyncBarrier,
    TaskBegin,
    TaskEnd,
    WaitTask,
    SetPosition,
    FilamentChange
>;

} // namespace hydra::gcode
