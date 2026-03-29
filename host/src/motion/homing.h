#pragma once

#include "../comms/mcu_proxy.h"
#include "../core/config.h"
#include "../drivers/tmc_config.h"
#include "protocol_defs.h"
#include <cstdint>
#include <functional>

namespace hydra::motion {

/**
 * Multi-phase homing sequence for a single axis group.
 *
 * Phases:
 *   1. Fast approach — move towards endstop at high speed
 *   2. Backoff — back off a few mm after endstop triggers
 *   3. Slow probe — slow approach for precise position
 *   4. Set position — record position as 0
 *
 * CoreXY note: X and Y endstops are on the A and B motors, so
 * homing "X" means moving both A and B toward the X endstop,
 * and "Y" means moving A and B toward the Y endstop.
 */
class HomingSequence {
public:
    enum class Phase {
        Idle,
        FastApproach,
        WaitFastTrigger,
        Backoff,
        WaitBackoff,
        SlowProbe,
        WaitSlowTrigger,
        SetPosition,
        Done,
        Failed,
    };

    struct AxisConfig {
        uint8_t channel_a;       /* Primary stepper channel */
        uint8_t channel_b;       /* Secondary channel (for CoreXY), or 0xFF if none */
        uint8_t channel_c;       /* Tertiary channel (for 3-point Z), or 0xFF if none */
        uint8_t endstop_bit;     /* Endstop bitmask (1=X, 2=Y, 4=Z) */
        double fast_speed;       /* mm/s */
        double slow_speed;       /* mm/s */
        double backoff_mm;       /* mm */
        double max_travel_mm;    /* mm before giving up */
        double steps_per_mm;
        bool invert_dir;         /* True if endstop is in positive direction */
        bool sensorless;         /* True = use TMC2209 StallGuard instead of physical endstops */
        uint8_t stall_threshold; /* StallGuard threshold (0-255, only if sensorless) */
    };

    HomingSequence(comms::McuProxy& mcu, const AxisConfig& config,
                   drivers::TmcConfig* tmc = nullptr);

    /** Start the homing sequence. */
    void start();

    /** Call once per control loop tick. Returns true when complete (done or failed). */
    bool tick();

    Phase phase() const { return phase_; }
    bool succeeded() const { return phase_ == Phase::Done; }

private:
    void queue_move(double distance_mm, double speed_mm_s);
    void enter_phase(Phase phase);

    comms::McuProxy& mcu_;
    drivers::TmcConfig* tmc_;  /* Non-null if sensorless homing available */
    AxisConfig config_;
    Phase phase_ = Phase::Idle;
    uint32_t phase_ticks_ = 0;
    uint32_t timeout_ticks_ = 0;
};

/**
 * Coordinates homing of all axes in sequence: Z first (safety), then XY.
 */
class HomingController {
public:
    HomingController(comms::McuProxy& mcu, const Config& config,
                     drivers::TmcConfig* tmc = nullptr);

    /** Start homing all axes. */
    void start(bool x, bool y, bool z);

    /** Tick the state machine. Returns true when all requested axes are done. */
    bool tick();

    bool succeeded() const { return succeeded_; }
    bool is_active() const { return active_; }

private:
    comms::McuProxy& mcu_;
    const Config& config_;
    drivers::TmcConfig* tmc_;

    bool active_ = false;
    bool succeeded_ = false;

    /* Which axes to home */
    bool home_x_ = false, home_y_ = false, home_z_ = false;

    enum class Stage {
        HomingZ,
        HomingX,
        HomingY,
        Done,
    };
    Stage stage_ = Stage::Done;

    std::unique_ptr<HomingSequence> current_seq_;
    void advance_stage();
};

} // namespace hydra::motion
