#include "homing.h"
#include <spdlog/spdlog.h>
#include <cmath>

namespace hydra::motion {

// ─── HomingSequence ──────────────────────────────────────────────────

HomingSequence::HomingSequence(comms::McuProxy& mcu, const AxisConfig& config)
    : mcu_(mcu), config_(config) {}

void HomingSequence::start() {
    enter_phase(Phase::FastApproach);
}

void HomingSequence::enter_phase(Phase new_phase) {
    phase_ = new_phase;
    phase_ticks_ = 0;

    switch (new_phase) {
    case Phase::FastApproach: {
        /* Enable endstop monitoring on the MCU */
        mcu_.endstop_home(config_.channel_a, config_.endstop_bit);
        if (config_.channel_b != 0xFF) {
            mcu_.endstop_home(config_.channel_b, config_.endstop_bit);
        }
        /* Queue a long move towards the endstop */
        double dir = config_.invert_dir ? 1.0 : -1.0;
        queue_move(dir * config_.max_travel_mm, config_.fast_speed);
        /* Timeout: max_travel / speed * 1000 ticks/s + margin */
        timeout_ticks_ = static_cast<uint32_t>(
            (config_.max_travel_mm / config_.fast_speed) * 1200);
        phase_ = Phase::WaitFastTrigger;
        break;
    }
    case Phase::Backoff: {
        /* Disable endstop monitoring during backoff */
        mcu_.endstop_home(config_.channel_a, 0);
        if (config_.channel_b != 0xFF) {
            mcu_.endstop_home(config_.channel_b, 0);
        }
        /* Move away from endstop */
        double dir = config_.invert_dir ? -1.0 : 1.0;
        queue_move(dir * config_.backoff_mm, config_.fast_speed);
        /* Timeout: backoff / speed * 1000 + margin */
        timeout_ticks_ = static_cast<uint32_t>(
            (config_.backoff_mm / config_.fast_speed) * 1500 + 200);
        phase_ = Phase::WaitBackoff;
        break;
    }
    case Phase::SlowProbe: {
        /* Re-enable endstop monitoring for precise probe */
        mcu_.endstop_home(config_.channel_a, config_.endstop_bit);
        if (config_.channel_b != 0xFF) {
            mcu_.endstop_home(config_.channel_b, config_.endstop_bit);
        }
        /* Slow approach */
        double dir = config_.invert_dir ? 1.0 : -1.0;
        queue_move(dir * (config_.backoff_mm * 2), config_.slow_speed);
        timeout_ticks_ = static_cast<uint32_t>(
            (config_.backoff_mm * 2 / config_.slow_speed) * 1200);
        phase_ = Phase::WaitSlowTrigger;
        break;
    }
    case Phase::SetPosition: {
        /* Disable endstop monitoring */
        mcu_.endstop_home(config_.channel_a, 0);
        if (config_.channel_b != 0xFF) {
            mcu_.endstop_home(config_.channel_b, 0);
        }
        /* Reset step position to 0 */
        mcu_.reset_step_clock(config_.channel_a, 0);
        if (config_.channel_b != 0xFF) {
            mcu_.reset_step_clock(config_.channel_b, 0);
        }
        phase_ = Phase::Done;
        break;
    }
    default:
        break;
    }
}

bool HomingSequence::tick() {
    phase_ticks_++;

    switch (phase_) {
    case Phase::WaitFastTrigger: {
        /* Poll for endstop trigger */
        auto query = mcu_.endstop_query();
        if (query && (query->trigger_flags &
                      ((1 << config_.channel_a) |
                       (config_.channel_b != 0xFF ? (1 << config_.channel_b) : 0)))) {
            spdlog::debug("Homing: fast approach triggered");
            enter_phase(Phase::Backoff);
            return false;
        }
        if (phase_ticks_ > timeout_ticks_) {
            spdlog::error("Homing: fast approach timeout — endstop not reached");
            mcu_.endstop_home(config_.channel_a, 0);
            if (config_.channel_b != 0xFF) mcu_.endstop_home(config_.channel_b, 0);
            phase_ = Phase::Failed;
            return true;
        }
        return false;
    }
    case Phase::WaitBackoff: {
        /* Wait for backoff move to complete (queue drains) */
        bool a_idle = mcu_.last_status().queue_depth[config_.channel_a] == 0;
        bool b_idle = config_.channel_b == 0xFF ||
                      mcu_.last_status().queue_depth[config_.channel_b] == 0;
        if (a_idle && b_idle) {
            /* Small delay to let motors settle */
            if (phase_ticks_ > 50) {
                enter_phase(Phase::SlowProbe);
            }
            return false;
        }
        if (phase_ticks_ > timeout_ticks_) {
            spdlog::error("Homing: backoff timeout");
            phase_ = Phase::Failed;
            return true;
        }
        return false;
    }
    case Phase::WaitSlowTrigger: {
        auto query = mcu_.endstop_query();
        if (query && (query->trigger_flags &
                      ((1 << config_.channel_a) |
                       (config_.channel_b != 0xFF ? (1 << config_.channel_b) : 0)))) {
            spdlog::debug("Homing: slow probe triggered — position locked");
            enter_phase(Phase::SetPosition);
            return false;
        }
        if (phase_ticks_ > timeout_ticks_) {
            spdlog::error("Homing: slow probe timeout — endstop not reached");
            mcu_.endstop_home(config_.channel_a, 0);
            if (config_.channel_b != 0xFF) mcu_.endstop_home(config_.channel_b, 0);
            phase_ = Phase::Failed;
            return true;
        }
        return false;
    }
    case Phase::Done:
    case Phase::Failed:
        return true;
    default:
        return false;
    }
}

void HomingSequence::queue_move(double distance_mm, double speed_mm_s) {
    int steps = static_cast<int>(std::abs(distance_mm) * config_.steps_per_mm);
    if (steps == 0) return;

    uint32_t interval = static_cast<uint32_t>(1000000.0 / (speed_mm_s * config_.steps_per_mm));
    if (interval < 10) interval = 10;

    uint8_t dir = distance_mm < 0 ? 1 : 0;

    mcu_.set_step_dir(config_.channel_a, dir);
    mcu_.queue_step(config_.channel_a, interval, static_cast<uint16_t>(steps), 0);

    if (config_.channel_b != 0xFF) {
        mcu_.set_step_dir(config_.channel_b, dir);
        mcu_.queue_step(config_.channel_b, interval, static_cast<uint16_t>(steps), 0);
    }
}

// ─── HomingController ────────────────────────────────────────────────

HomingController::HomingController(comms::McuProxy& mcu, const Config& config)
    : mcu_(mcu), config_(config) {}

void HomingController::start(bool x, bool y, bool z) {
    home_x_ = x;
    home_y_ = y;
    home_z_ = z;
    active_ = true;
    succeeded_ = true;

    /* Home Z first (move bed down for safety), then X, then Y */
    if (home_z_) {
        stage_ = Stage::HomingZ;
    } else if (home_x_) {
        stage_ = Stage::HomingX;
    } else if (home_y_) {
        stage_ = Stage::HomingY;
    } else {
        stage_ = Stage::Done;
        active_ = false;
        return;
    }

    advance_stage();
}

void HomingController::advance_stage() {
    HomingSequence::AxisConfig axis_cfg;

    switch (stage_) {
    case Stage::HomingZ:
        spdlog::info("Homing Z axis");
        axis_cfg.channel_a = STEPPER_Z;
        axis_cfg.channel_b = 0xFF; /* Single motor */
        axis_cfg.endstop_bit = 0x04; /* Z endstop = bit 2 */
        axis_cfg.fast_speed = config_.homing.fast_speed_z;
        axis_cfg.slow_speed = config_.homing.slow_speed_z;
        axis_cfg.backoff_mm = config_.homing.backoff_mm;
        axis_cfg.max_travel_mm = config_.homing.max_travel_z;
        axis_cfg.steps_per_mm = config_.steps_per_mm_z;
        axis_cfg.invert_dir = false;
        break;

    case Stage::HomingX:
        spdlog::info("Homing X axis (CoreXY A+B)");
        /*
         * CoreXY homing: X endstop is triggered by moving
         * both A and B in the same direction (A=X+Y, B=X-Y).
         * Moving X negative → A negative, B negative.
         */
        axis_cfg.channel_a = STEPPER_COREXY_A;
        axis_cfg.channel_b = STEPPER_COREXY_B;
        axis_cfg.endstop_bit = 0x01; /* X endstop = bit 0 */
        axis_cfg.fast_speed = config_.homing.fast_speed_xy;
        axis_cfg.slow_speed = config_.homing.slow_speed_xy;
        axis_cfg.backoff_mm = config_.homing.backoff_mm;
        axis_cfg.max_travel_mm = config_.homing.max_travel_xy;
        axis_cfg.steps_per_mm = config_.steps_per_mm_xy;
        axis_cfg.invert_dir = false;
        break;

    case Stage::HomingY:
        spdlog::info("Homing Y axis (CoreXY A-B)");
        /*
         * CoreXY Y homing: A moves negative, B moves positive
         * (since A=X+Y, B=X-Y, pure -Y → A negative, B positive).
         * We home A toward Y endstop; B gets the inverted direction.
         * However with the endstop-triggered halt model, we need
         * both channels moving, so we send them separately.
         * For now, use A channel only — B will be synced after.
         */
        axis_cfg.channel_a = STEPPER_COREXY_A;
        axis_cfg.channel_b = 0xFF; /* Handle B manually for Y */
        axis_cfg.endstop_bit = 0x02; /* Y endstop = bit 1 */
        axis_cfg.fast_speed = config_.homing.fast_speed_xy;
        axis_cfg.slow_speed = config_.homing.slow_speed_xy;
        axis_cfg.backoff_mm = config_.homing.backoff_mm;
        axis_cfg.max_travel_mm = config_.homing.max_travel_xy;
        axis_cfg.steps_per_mm = config_.steps_per_mm_xy;
        axis_cfg.invert_dir = false;
        break;

    case Stage::Done:
        return;
    }

    current_seq_ = std::make_unique<HomingSequence>(mcu_, axis_cfg);
    current_seq_->start();
}

bool HomingController::tick() {
    if (!active_) return true;

    if (!current_seq_) {
        active_ = false;
        return true;
    }

    bool done = current_seq_->tick();
    if (!done) return false;

    if (!current_seq_->succeeded()) {
        spdlog::error("Homing failed at stage {}", static_cast<int>(stage_));
        succeeded_ = false;
        active_ = false;
        return true;
    }

    /* Advance to next stage */
    switch (stage_) {
    case Stage::HomingZ:
        if (home_x_) {
            stage_ = Stage::HomingX;
            advance_stage();
            return false;
        } else if (home_y_) {
            stage_ = Stage::HomingY;
            advance_stage();
            return false;
        }
        break;
    case Stage::HomingX:
        if (home_y_) {
            stage_ = Stage::HomingY;
            advance_stage();
            return false;
        }
        break;
    case Stage::HomingY:
        break;
    case Stage::Done:
        break;
    }

    /* All stages complete */
    spdlog::info("Homing complete — all axes homed successfully");
    stage_ = Stage::Done;
    active_ = false;
    return true;
}

} // namespace hydra::motion
