#include "engine.h"
#include "../motion/corexy.h"
#include "../motion/step_compress.h"
#include <spdlog/spdlog.h>
#include <thread>
#include <chrono>

namespace hydra {

Engine::Engine(const Config& config) : config_(config) {
    spdlog::info("Initializing print engine");

    /* Create MCU proxy */
    mcu_ = std::make_unique<comms::McuProxy>(config);

    /* Create heaters */
    thermal::Heater::Config n0_cfg;
    n0_cfg.name = "nozzle0";
    n0_cfg.pwm_channel = PWM_HEATER_NOZZLE_0;
    n0_cfg.adc_channel = ADC_THERM_NOZZLE_0;
    n0_cfg.pid_gains = {config.nozzle_pid.kp, config.nozzle_pid.ki, config.nozzle_pid.kd};
    n0_cfg.max_temp = 300.0;
    heaters_[0] = std::make_unique<thermal::Heater>(n0_cfg);

    thermal::Heater::Config n1_cfg = n0_cfg;
    n1_cfg.name = "nozzle1";
    n1_cfg.pwm_channel = PWM_HEATER_NOZZLE_1;
    n1_cfg.adc_channel = ADC_THERM_NOZZLE_1;
    heaters_[1] = std::make_unique<thermal::Heater>(n1_cfg);

    thermal::Heater::Config bed_cfg;
    bed_cfg.name = "bed";
    bed_cfg.pwm_channel = PWM_HEATER_BED;
    bed_cfg.adc_channel = ADC_THERM_BED;
    bed_cfg.pid_gains = {config.bed_pid.kp, config.bed_pid.ki, config.bed_pid.kd};
    bed_cfg.max_temp = 120.0;
    heaters_[2] = std::make_unique<thermal::Heater>(bed_cfg);

    /* Create planners */
    motion::Planner::Config planner_cfg;
    planner_cfg.max_velocity = config.max_velocity;
    planner_cfg.max_acceleration = config.max_acceleration;
    planner_cfg.max_z_velocity = config.max_z_velocity;
    planner_cfg.junction_deviation = config.junction_deviation;
    planner_[0] = std::make_unique<motion::Planner>(planner_cfg);
    planner_[1] = std::make_unique<motion::Planner>(planner_cfg);
}

Engine::~Engine() {
    shutdown();
}

void Engine::shutdown() {
    running_ = false;
}

void Engine::run() {
    running_ = true;
    spdlog::info("Engine main loop started");

    /* Connect to MCU */
    if (!mcu_->connect()) {
        spdlog::warn("MCU connection failed — running in offline mode");
    }

    last_tick_ = std::chrono::steady_clock::now();

    while (running_) {
        auto now = std::chrono::steady_clock::now();
        double dt_s = std::chrono::duration<double>(now - last_tick_).count();
        last_tick_ = now;

        control_loop_tick(dt_s);

        /* ~1kHz control loop */
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    spdlog::info("Engine main loop stopped");
}

void Engine::control_loop_tick(double dt_s) {
    tick_count_++;

    /* Periodic clock sync */
    if (mcu_->is_connected() && (tick_count_ % clock_sync_interval_) == 0) {
        mcu_->sync_clock();
    }

    /* Poll MCU status */
    if (mcu_->is_connected() && (tick_count_ % status_poll_interval_) == 0) {
        mcu_->poll_status();
    }

    /* Run thermal PID */
    tick_thermal(dt_s);

    /* Safety checks */
    check_safety();

    /* State-specific logic */
    switch (state_.load()) {
    case PrinterState::Homing:
        tick_homing();
        break;
    case PrinterState::Printing:
        tick_printing();
        break;
    case PrinterState::Heating: {
        /* Check if all heaters are at target */
        bool all_ready = true;
        for (auto& h : heaters_) {
            if (h && h->target() > 0 && !h->at_target()) {
                all_ready = false;
            }
        }
        if (all_ready) {
            spdlog::info("All heaters at target — starting print");
            state_ = PrinterState::Printing;
        }
        break;
    }
    default:
        break;
    }

    /* Heartbeat to MCU */
    if (mcu_->is_connected() && (tick_count_ % 200) == 0) {
        mcu_->heartbeat();
    }
}

void Engine::tick_thermal(double dt_s) {
    if (!mcu_->is_connected()) return;

    const auto& status = mcu_->last_status();

    for (int i = 0; i < 3; i++) {
        if (!heaters_[i]) continue;

        uint16_t adc_raw = status.adc_raw[i];
        uint16_t pwm_duty = heaters_[i]->update(adc_raw, dt_s);

        /* Send PWM to MCU */
        mcu_->set_heater_pwm(static_cast<uint8_t>(i), pwm_duty);

        /* Check for faults */
        if (heaters_[i]->has_fault()) {
            spdlog::error("Heater {} fault: {}", heaters_[i]->name(),
                         heaters_[i]->fault_reason());
            emergency_stop();
        }
    }
}

void Engine::tick_printing() {
    if (!sync_) return;

    /* Feed moves from sync engine to planner, then to MCU step queues */
    if (!mcu_->queue_has_space(STEPPER_COREXY_A)) return;

    /* Get next command from sync engine (returns command for any nozzle) */
    auto tagged = sync_->next();
    if (tagged) {
        int nozzle = tagged->nozzle_id;
        auto& command = tagged->command;

        /* Handle move commands */
        if (auto* move = std::get_if<gcode::MoveCommand>(&command)) {
            motion::CartesianPos target;
            if (move->x) target.x = *move->x;
            if (move->y) target.y = *move->y;
            if (move->z) target.z = *move->z;
            if (move->e) target.e = *move->e;

            double feedrate = move->f.value_or(config_.max_velocity);
            bool is_travel = !move->e.has_value();

            planner_[nozzle]->push(target, feedrate, is_travel);
        }
        else if (auto* temp = std::get_if<gcode::TempCommand>(&command)) {
            if (temp->target == gcode::TempCommand::Bed) {
                heaters_[2]->set_target(temp->temp_c);
            } else {
                int idx = (temp->nozzle_id >= 0) ? temp->nozzle_id : nozzle;
                if (idx >= 0 && idx <= 1) {
                    heaters_[idx]->set_target(temp->temp_c);
                }
            }
        }
        else if (auto* fan = std::get_if<gcode::FanCommand>(&command)) {
            int fan_channel = PWM_FAN_PART_0 + nozzle;
            uint16_t duty = static_cast<uint16_t>(fan->speed * 65535 / 255);
            mcu_->set_fan_pwm(static_cast<uint8_t>(fan_channel), duty);
        }
    }

    /* Drain planners to MCU step queues */
    for (int nozzle = 0; nozzle < 2; nozzle++) {
        auto move = planner_[nozzle]->pop();
        if (!move) continue;

        /* Convert Cartesian move to CoreXY motor positions */
        auto start_motors = motion::cartesian_to_corexy(move->start.x, move->start.y);
        auto end_motors = motion::cartesian_to_corexy(move->end.x, move->end.y);

        double a_steps = (end_motors.a - start_motors.a) * config_.steps_per_mm_xy;
        double b_steps = (end_motors.b - start_motors.b) * config_.steps_per_mm_xy;
        double z_steps = (move->end.z - move->start.z) * config_.steps_per_mm_z;
        double e_steps = (move->end.e - move->start.e) * config_.steps_per_mm_e;

        /* Queue step segments for each axis that needs to move */
        struct AxisMove {
            uint8_t channel;
            double steps;
            double steps_per_mm;
        } axes[] = {
            {STEPPER_COREXY_A, a_steps, config_.steps_per_mm_xy},
            {STEPPER_COREXY_B, b_steps, config_.steps_per_mm_xy},
            {STEPPER_Z,        z_steps, config_.steps_per_mm_z},
            {static_cast<uint8_t>(STEPPER_EXTRUDER_0 + nozzle), e_steps, config_.steps_per_mm_e},
        };

        for (auto& axis : axes) {
            int abs_steps = static_cast<int>(std::abs(axis.steps));
            if (abs_steps == 0) continue;

            /* Set direction */
            mcu_->set_step_dir(axis.channel, axis.steps < 0 ? 1 : 0);

            /* Compute step interval from cruise speed */
            double speed_steps = move->cruise_speed * axis.steps_per_mm;
            if (speed_steps < 1.0) speed_steps = 1.0;
            uint32_t interval = static_cast<uint32_t>(1000000.0 / speed_steps);
            if (interval < 10) interval = 10;

            /* Simple constant-velocity segment for now */
            mcu_->queue_step(axis.channel, interval,
                           static_cast<uint16_t>(abs_steps), 0);
        }
    }

    /* Check if print is done */
    if (sync_->done()) {
        spdlog::info("Print complete");
        state_ = PrinterState::Finishing;

        /* Cool down */
        for (auto& h : heaters_) {
            if (h) h->set_target(0.0);
        }

        state_ = PrinterState::Idle;
    }
}

void Engine::tick_homing() {
    /*
     * Homing sequence:
     * 1. Move towards endstop at moderate speed
     * 2. When endstop triggers, stop and back off
     * 3. Move towards endstop slowly for precision
     * 4. Set position to 0
     *
     * For now, this is a placeholder that transitions to Idle.
     * Real implementation requires endstop monitoring via MCU status.
     */
    const auto& status = mcu_->last_status();

    /* Check if all endstops have been hit (simplified) */
    bool x_homed = (status.endstop_state & 0x01) != 0;
    bool y_homed = (status.endstop_state & 0x02) != 0;
    bool z_homed = (status.endstop_state & 0x04) != 0;

    if (x_homed && y_homed && z_homed) {
        spdlog::info("Homing complete");
        state_ = PrinterState::Idle;
    }
}

void Engine::check_safety() {
    if (!mcu_->is_connected()) return;

    const auto& status = mcu_->last_status();

    if (status.flags & STATUS_FLAG_OVERTEMP) {
        spdlog::error("MCU reports overtemperature!");
        emergency_stop();
    }
    if (status.flags & STATUS_FLAG_ESTOP) {
        spdlog::error("MCU reports emergency stop active");
        state_ = PrinterState::Error;
    }
    if (status.flags & STATUS_FLAG_WATCHDOG) {
        spdlog::error("MCU reports watchdog timeout");
        state_ = PrinterState::Error;
    }
}

bool Engine::start_print(const std::string& gcode_base_path) {
    if (state_ != PrinterState::Idle) {
        spdlog::warn("Cannot start print: not idle (state={})", static_cast<int>(state_.load()));
        return false;
    }

    spdlog::info("Starting print: {}", gcode_base_path);

    /* Open dual gcode streams */
    stream_ = std::make_unique<gcode::Stream>();
    if (!stream_->open(gcode_base_path)) {
        spdlog::error("Failed to open G-code files: {}", gcode_base_path);
        return false;
    }

    /* Create sync engine */
    sync_ = std::make_unique<gcode::SyncEngine>(*stream_);

    /* Clear planners */
    planner_[0]->clear();
    planner_[1]->clear();

    state_ = PrinterState::Heating;
    return true;
}

void Engine::pause() {
    if (state_ == PrinterState::Printing) {
        spdlog::info("Pausing print");
        state_ = PrinterState::Paused;
    }
}

void Engine::resume() {
    if (state_ == PrinterState::Paused) {
        spdlog::info("Resuming print");
        state_ = PrinterState::Printing;
    }
}

void Engine::cancel() {
    spdlog::info("Cancelling print");
    sync_.reset();
    stream_.reset();
    planner_[0]->clear();
    planner_[1]->clear();

    /* Cool down heaters */
    for (auto& h : heaters_) {
        if (h) h->set_target(0.0);
    }

    state_ = PrinterState::Idle;
}

void Engine::home_all() {
    if (state_ != PrinterState::Idle) return;
    spdlog::info("Homing all axes");
    state_ = PrinterState::Homing;

    /* Send slow moves towards endstops */
    double home_speed = 50.0; /* mm/s */
    double step_interval = 1000000.0 / (home_speed * config_.steps_per_mm_xy);

    /* Move X, Y, Z towards negative endstops */
    for (uint8_t ch : {STEPPER_COREXY_A, STEPPER_COREXY_B}) {
        mcu_->set_step_dir(ch, 1); /* Negative direction */
        mcu_->queue_step(ch, static_cast<uint32_t>(step_interval), 10000, 0);
    }
    mcu_->set_step_dir(STEPPER_Z, 1);
    double z_interval = 1000000.0 / (5.0 * config_.steps_per_mm_z); /* 5mm/s for Z */
    mcu_->queue_step(STEPPER_Z, static_cast<uint32_t>(z_interval), 5000, 0);
}

void Engine::jog(char axis, double distance_mm, double speed_mm_s) {
    if (state_ != PrinterState::Idle) return;
    spdlog::debug("Jog {}={:.1f}mm @ {:.0f}mm/s", axis, distance_mm, speed_mm_s);

    uint8_t channel;
    double steps_per_mm;

    switch (axis) {
    case 'X': case 'x':
        /* CoreXY: X jog moves both A and B by same amount */
        channel = STEPPER_COREXY_A;
        steps_per_mm = config_.steps_per_mm_xy;
        break;
    case 'Y': case 'y':
        channel = STEPPER_COREXY_A; /* Will handle B separately */
        steps_per_mm = config_.steps_per_mm_xy;
        break;
    case 'Z': case 'z':
        channel = STEPPER_Z;
        steps_per_mm = config_.steps_per_mm_z;
        break;
    case 'E': case 'e':
        channel = STEPPER_EXTRUDER_0;
        steps_per_mm = config_.steps_per_mm_e;
        break;
    default:
        return;
    }

    int steps = static_cast<int>(std::abs(distance_mm * steps_per_mm));
    if (steps == 0) return;

    uint32_t interval = static_cast<uint32_t>(1000000.0 / (speed_mm_s * steps_per_mm));
    if (interval < 10) interval = 10;

    uint8_t dir = distance_mm < 0 ? 1 : 0;

    if (axis == 'X' || axis == 'x') {
        /* CoreXY X: A = +distance, B = +distance */
        mcu_->set_step_dir(STEPPER_COREXY_A, dir);
        mcu_->queue_step(STEPPER_COREXY_A, interval, static_cast<uint16_t>(steps), 0);
        mcu_->set_step_dir(STEPPER_COREXY_B, dir);
        mcu_->queue_step(STEPPER_COREXY_B, interval, static_cast<uint16_t>(steps), 0);
    } else if (axis == 'Y' || axis == 'y') {
        /* CoreXY Y: A = +distance, B = -distance */
        mcu_->set_step_dir(STEPPER_COREXY_A, dir);
        mcu_->queue_step(STEPPER_COREXY_A, interval, static_cast<uint16_t>(steps), 0);
        mcu_->set_step_dir(STEPPER_COREXY_B, dir ? 0 : 1);
        mcu_->queue_step(STEPPER_COREXY_B, interval, static_cast<uint16_t>(steps), 0);
    } else {
        mcu_->set_step_dir(channel, dir);
        mcu_->queue_step(channel, interval, static_cast<uint16_t>(steps), 0);
    }
}

void Engine::set_nozzle_temp(int nozzle, double temp_c) {
    if (nozzle >= 0 && nozzle <= 1 && heaters_[nozzle]) {
        heaters_[nozzle]->set_target(temp_c);
    }
}

void Engine::set_bed_temp(double temp_c) {
    if (heaters_[2]) {
        heaters_[2]->set_target(temp_c);
    }
}

void Engine::set_fan_speed(int fan, int percent) {
    spdlog::debug("Set fan {} speed: {}%", fan, percent);
    uint16_t duty = static_cast<uint16_t>(percent * 65535 / 100);
    uint8_t channel = static_cast<uint8_t>(PWM_FAN_PART_0 + fan);
    mcu_->set_fan_pwm(channel, duty);
}

void Engine::emergency_stop() {
    spdlog::error("EMERGENCY STOP");
    state_ = PrinterState::Error;

    /* Kill all heaters */
    for (auto& h : heaters_) {
        if (h) h->kill();
    }

    /* Tell MCU to stop everything */
    mcu_->emergency_stop();
}

double Engine::progress() const {
    /* Approximate from sync engine */
    return 0.0;
}

double Engine::nozzle_temp(int nozzle) const {
    if (nozzle >= 0 && nozzle <= 1 && heaters_[nozzle]) {
        return heaters_[nozzle]->current_temp();
    }
    return 0.0;
}

double Engine::bed_temp() const {
    if (heaters_[2]) {
        return heaters_[2]->current_temp();
    }
    return 0.0;
}

} // namespace hydra
