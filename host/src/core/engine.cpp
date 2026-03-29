#include "engine.h"
#include "../motion/corexy.h"
#include "../motion/step_compress.h"
#include <spdlog/spdlog.h>
#include <thread>
#include <chrono>

namespace hydra {

Engine::Engine(const Config& config) : config_(config) {
    spdlog::info("Initializing print engine (multi-nozzle valve array)");

    /* Create MCU proxy */
    mcu_ = std::make_unique<comms::McuProxy>(config);

    /* Create heaters: manifold + bed */
    thermal::Heater::Config manifold_cfg;
    manifold_cfg.name = "manifold";
    manifold_cfg.pwm_channel = PWM_HEATER_MANIFOLD;
    manifold_cfg.adc_channel = ADC_THERM_MANIFOLD;
    manifold_cfg.pid_gains = {config.nozzle_pid.kp, config.nozzle_pid.ki, config.nozzle_pid.kd};
    manifold_cfg.max_temp = 300.0;
    heaters_[0] = std::make_unique<thermal::Heater>(manifold_cfg);

    thermal::Heater::Config bed_cfg;
    bed_cfg.name = "bed";
    bed_cfg.pwm_channel = PWM_HEATER_BED;
    bed_cfg.adc_channel = ADC_THERM_BED;
    bed_cfg.pid_gains = {config.bed_pid.kp, config.bed_pid.ki, config.bed_pid.kd};
    bed_cfg.max_temp = 120.0;
    heaters_[1] = std::make_unique<thermal::Heater>(bed_cfg);

    /* Create planner (single — one gcode stream) */
    motion::Planner::Config planner_cfg;
    planner_cfg.max_velocity = config.max_velocity;
    planner_cfg.max_acceleration = config.max_acceleration;
    planner_cfg.max_z_velocity = config.max_z_velocity;
    planner_cfg.junction_deviation = config.junction_deviation;
    planner_ = std::make_unique<motion::Planner>(planner_cfg);

    /* Create frame decomposer */
    decomposer_ = std::make_unique<motion::FrameDecomposer>(config);

    /* Create TMC driver configurator */
    tmc_config_ = std::make_unique<drivers::TmcConfig>(*mcu_);

    /* Create file manager */
    file_manager_ = std::make_unique<files::FileManager>(config.gcode_dir);

    /* Create checkpoint manager */
    checkpoint_mgr_ = std::make_unique<CheckpointManager>(config.log_dir);

    /* Create OTA update manager */
    ota_manager_ = std::make_unique<OtaManager>();
}

Engine::~Engine() {
    shutdown();
}

void Engine::shutdown() {
    running_ = false;
}

void Engine::configure_drivers() {
    spdlog::info("Configuring TMC2209 stepper drivers");

    std::array<drivers::TmcDriverConfig, STEPPER_COUNT> configs;

    /* CoreXY A,B (channels 0,1) */
    for (int i = 0; i < 2; i++) {
        configs[i].driver_id = static_cast<uint8_t>(i);
        configs[i].run_current_ma = config_.tmc.xy_run_current_ma;
        configs[i].hold_current_ma = config_.tmc.xy_hold_current_ma;
        configs[i].microsteps = config_.tmc.microsteps;
        configs[i].stealthchop = config_.tmc.stealthchop;
    }

    /* Z1, Z2, Z3 (channels 2,3,4) */
    for (int i = 2; i < 5; i++) {
        configs[i].driver_id = static_cast<uint8_t>(i);
        configs[i].run_current_ma = config_.tmc.z_run_current_ma;
        configs[i].hold_current_ma = config_.tmc.z_hold_current_ma;
        configs[i].microsteps = config_.tmc.microsteps;
        configs[i].stealthchop = config_.tmc.stealthchop;
    }

    /* Extruder (channel 5) */
    configs[5].driver_id = 5;
    configs[5].run_current_ma = config_.tmc.e_run_current_ma;
    configs[5].hold_current_ma = config_.tmc.e_hold_current_ma;
    configs[5].microsteps = config_.tmc.microsteps;
    configs[5].stealthchop = config_.tmc.stealthchop;

    /* Spare (channel 6) — configure but unused */
    configs[6].driver_id = 6;
    configs[6].run_current_ma = 0;
    configs[6].hold_current_ma = 0;
    configs[6].microsteps = config_.tmc.microsteps;
    configs[6].stealthchop = config_.tmc.stealthchop;

    if (!tmc_config_->configure_all(configs)) {
        spdlog::warn("Some TMC drivers failed to configure — check wiring");
    }
}

void Engine::run() {
    running_ = true;
    spdlog::info("Engine main loop started");

    /* Connect to MCU */
    if (!mcu_->connect()) {
        spdlog::warn("MCU connection failed — running in offline mode");
    } else {
        configure_drivers();
    }

    /* Check for pending checkpoint */
    if (checkpoint_mgr_->exists()) {
        spdlog::info("Power-loss checkpoint found — use resume_print() to continue");
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
    case PrinterState::FilamentChange:
        tick_filament_change();
        break;
    case PrinterState::Heating: {
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

    /* Periodic checkpoint during printing */
    if (state_ == PrinterState::Printing &&
        (tick_count_ % checkpoint_interval_) == 0) {
        save_checkpoint();
    }
}

void Engine::tick_thermal(double dt_s) {
    if (!mcu_->is_connected()) return;

    const auto& status = mcu_->last_status();

    for (int i = 0; i < 2; i++) {
        if (!heaters_[i]) continue;

        uint16_t adc_raw = status.adc_raw[i];
        uint16_t pwm_duty = heaters_[i]->update(adc_raw, dt_s);
        mcu_->set_heater_pwm(static_cast<uint8_t>(i), pwm_duty);

        if (heaters_[i]->has_fault()) {
            spdlog::error("Heater {} fault: {}", heaters_[i]->name(),
                         heaters_[i]->fault_reason());
            emergency_stop();
        }
    }
}

void Engine::queue_decomposed_move(const motion::DecomposedMove& dm) {
    auto queue_axis = [this](uint8_t channel, const motion::DecomposedMove::AxisMove& axis) {
        int abs_steps = std::abs(axis.steps);
        if (abs_steps == 0) return;

        mcu_->set_step_dir(channel, axis.steps < 0 ? 1 : 0);

        uint32_t interval = 10;
        if (axis.speed_steps_s > 0) {
            interval = static_cast<uint32_t>(1000000.0 / axis.speed_steps_s);
            if (interval < 10) interval = 10;
        }

        mcu_->queue_step(channel, interval, static_cast<uint16_t>(abs_steps), 0);
    };

    queue_axis(STEPPER_COREXY_A, dm.corexy_a);
    queue_axis(STEPPER_COREXY_B, dm.corexy_b);
    queue_axis(STEPPER_Z1,       dm.z1);
    queue_axis(STEPPER_Z2,       dm.z2);
    queue_axis(STEPPER_Z3,       dm.z3);
    queue_axis(STEPPER_EXTRUDER, dm.extruder);
}

void Engine::send_valve_state(uint8_t mask) {
    if (valve_mask_ == mask) return;
    valve_mask_ = mask;
    mcu_->send_valve_set(mask);
    spdlog::debug("Valve mask set to 0b{:04b}", mask);
}

void Engine::tick_printing() {
    if (!stream_) return;

    if (!mcu_->queue_has_space(STEPPER_COREXY_A)) return;

    /* Get next command from stream */
    auto command = stream_->next();
    if (command) {
        if (auto* move = std::get_if<gcode::MoveCommand>(&*command)) {
            motion::CartesianPos target;
            if (move->x) target.x = *move->x;
            if (move->y) target.y = *move->y;
            if (move->z) target.z = *move->z;
            if (move->e) target.e = *move->e;

            double feedrate = move->f.value_or(config_.max_velocity);
            bool is_travel = !move->e.has_value();

            planner_->push(target, feedrate, is_travel, valve_mask_);
        }
        else if (auto* temp = std::get_if<gcode::TempCommand>(&*command)) {
            if (temp->target == gcode::TempCommand::Bed) {
                heaters_[1]->set_target(temp->temp_c);
            } else {
                /* Manifold heater (single shared chamber) */
                heaters_[0]->set_target(temp->temp_c);
            }
        }
        else if (auto* fan = std::get_if<gcode::FanCommand>(&*command)) {
            uint16_t duty = static_cast<uint16_t>(fan->speed * 65535 / 255);
            mcu_->set_fan_pwm(static_cast<uint8_t>(PWM_FAN_PART), duty);
        }
        else if (auto* home = std::get_if<gcode::HomeCommand>(&*command)) {
            spdlog::info("G28 in print stream — homing axes");
            home_axes(home->x, home->y, home->z);
        }
        else if (auto* valve = std::get_if<gcode::ValveSet>(&*command)) {
            send_valve_state(valve->mask);
        }
        else if (std::get_if<gcode::FilamentChange>(&*command)) {
            spdlog::info("M600 — filament change requested");
            state_ = PrinterState::FilamentChange;
            return;
        }
    }

    /* Drain planner through frame decomposer to MCU step queues */
    auto move = planner_->pop();
    if (move) {
        auto decomposed = decomposer_->decompose(*move);

        /* Update valve state if the move has a different mask */
        if (decomposed.valve_mask != valve_mask_) {
            send_valve_state(decomposed.valve_mask);
        }

        queue_decomposed_move(decomposed);
    }

    /* Check if print is done */
    if (stream_->eof() && planner_->queue_depth() == 0) {
        spdlog::info("Print complete");
        state_ = PrinterState::Finishing;

        /* Close all valves */
        send_valve_state(0);

        for (auto& h : heaters_) {
            if (h) h->set_target(0.0);
        }

        /* Clear checkpoint on successful completion */
        checkpoint_mgr_->clear();

        state_ = PrinterState::Idle;
    }
}

void Engine::tick_homing() {
    if (!homing_ctrl_) return;

    if (homing_ctrl_->tick()) {
        /* Homing complete */
        if (homing_ctrl_->succeeded()) {
            decomposer_->reset();
            spdlog::info("Homing successful");
        } else {
            spdlog::error("Homing failed");
        }
        homing_ctrl_.reset();
        state_ = PrinterState::Idle;
    }
}

void Engine::tick_filament_change() {
    /*
     * M600 filament change sequence:
     * 1. Close all valves
     * 2. Retract filament
     * 3. Move nozzle to park position
     * 4. Wait for user to confirm new filament loaded
     * 5. Prime extruder
     * 6. Resume print
     */
}

void Engine::filament_change_confirm() {
    if (state_ != PrinterState::FilamentChange) return;
    spdlog::info("Filament change confirmed — resuming print");

    /* Prime extruder: push a small amount of filament */
    double prime_mm = 30.0;
    double prime_speed = 3.0; /* mm/s */
    int steps = static_cast<int>(prime_mm * config_.steps_per_mm_e);
    uint32_t interval = static_cast<uint32_t>(1000000.0 / (prime_speed * config_.steps_per_mm_e));

    mcu_->set_step_dir(STEPPER_EXTRUDER, 0);
    mcu_->queue_step(STEPPER_EXTRUDER, interval, static_cast<uint16_t>(steps), 0);

    state_ = PrinterState::Printing;
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

void Engine::save_checkpoint() {
    if (!stream_ || !decomposer_) return;

    PrintCheckpoint cp;
    cp.gcode_path = current_gcode_path_;
    cp.line = stream_->line_number();

    auto pos = decomposer_->current_position();
    cp.pos_x = pos.x;
    cp.pos_y = pos.y;
    cp.pos_z = pos.z;
    cp.e = pos.e;

    cp.manifold_target = heaters_[0] ? heaters_[0]->target() : 0;
    cp.bed_target = heaters_[1] ? heaters_[1]->target() : 0;

    cp.valve_mask = valve_mask_;
    cp.layer = current_layer_;

    auto elapsed = std::chrono::steady_clock::now() - print_start_time_;
    cp.elapsed_s = std::chrono::duration<double>(elapsed).count();

    checkpoint_mgr_->save(cp);
}

bool Engine::start_print(const std::string& gcode_path) {
    if (state_ != PrinterState::Idle) {
        spdlog::warn("Cannot start print: not idle (state={})", static_cast<int>(state_.load()));
        return false;
    }

    spdlog::info("Starting print: {}", gcode_path);
    current_gcode_path_ = gcode_path;

    stream_ = std::make_unique<gcode::Stream>();
    if (!stream_->open(gcode_path)) {
        spdlog::error("Failed to open G-code file: {}", gcode_path);
        return false;
    }

    planner_->clear();
    decomposer_->reset();
    valve_mask_ = 0;
    current_layer_ = 0;
    print_start_time_ = std::chrono::steady_clock::now();

    state_ = PrinterState::Heating;
    return true;
}

bool Engine::resume_print() {
    PrintCheckpoint cp;
    if (!checkpoint_mgr_->load(cp)) {
        spdlog::warn("No checkpoint to resume from");
        return false;
    }

    spdlog::info("Resuming print from checkpoint: {} at layer {}", cp.gcode_path, cp.layer);

    /* Restore heater targets */
    if (heaters_[0]) heaters_[0]->set_target(cp.manifold_target);
    if (heaters_[1]) heaters_[1]->set_target(cp.bed_target);

    /* Open G-code stream and skip to checkpoint position */
    stream_ = std::make_unique<gcode::Stream>();
    if (!stream_->open(cp.gcode_path)) {
        spdlog::error("Failed to open G-code file for resume: {}", cp.gcode_path);
        return false;
    }

    stream_->skip_to(cp.line);

    planner_->clear();
    decomposer_->reset();
    valve_mask_ = cp.valve_mask;
    current_gcode_path_ = cp.gcode_path;
    current_layer_ = cp.layer;
    print_start_time_ = std::chrono::steady_clock::now();

    state_ = PrinterState::Heating;
    return true;
}

bool Engine::has_checkpoint() const {
    return checkpoint_mgr_->exists();
}

void Engine::pause() {
    if (state_ == PrinterState::Printing) {
        spdlog::info("Pausing print");
        save_checkpoint();
        send_valve_state(0); /* Close all valves on pause */
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
    stream_.reset();
    planner_->clear();
    decomposer_->reset();
    send_valve_state(0);

    for (auto& h : heaters_) {
        if (h) h->set_target(0.0);
    }

    checkpoint_mgr_->clear();
    state_ = PrinterState::Idle;
}

void Engine::home_all() {
    home_axes(true, true, true);
}

void Engine::home_axes(bool x, bool y, bool z) {
    if (state_ != PrinterState::Idle && state_ != PrinterState::Printing) return;
    spdlog::info("Homing axes: X={} Y={} Z={}", x, y, z);

    homing_ctrl_ = std::make_unique<motion::HomingController>(*mcu_, config_, tmc_config_.get());
    homing_ctrl_->start(x, y, z);
    state_ = PrinterState::Homing;
}

void Engine::jog(char axis, double distance_mm, double speed_mm_s) {
    if (state_ != PrinterState::Idle) return;
    spdlog::debug("Jog {}={:.1f}mm @ {:.0f}mm/s", axis, distance_mm, speed_mm_s);

    uint8_t channel;
    double steps_per_mm;

    switch (axis) {
    case 'X': case 'x':
        channel = STEPPER_COREXY_A;
        steps_per_mm = config_.steps_per_mm_xy;
        break;
    case 'Y': case 'y':
        channel = STEPPER_COREXY_A;
        steps_per_mm = config_.steps_per_mm_xy;
        break;
    case 'Z': case 'z':
        channel = STEPPER_Z1;
        steps_per_mm = config_.steps_per_mm_z;
        break;
    case 'E': case 'e':
        channel = STEPPER_EXTRUDER;
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
        mcu_->set_step_dir(STEPPER_COREXY_A, dir);
        mcu_->queue_step(STEPPER_COREXY_A, interval, static_cast<uint16_t>(steps), 0);
        mcu_->set_step_dir(STEPPER_COREXY_B, dir);
        mcu_->queue_step(STEPPER_COREXY_B, interval, static_cast<uint16_t>(steps), 0);
    } else if (axis == 'Y' || axis == 'y') {
        mcu_->set_step_dir(STEPPER_COREXY_A, dir);
        mcu_->queue_step(STEPPER_COREXY_A, interval, static_cast<uint16_t>(steps), 0);
        mcu_->set_step_dir(STEPPER_COREXY_B, dir ? 0 : 1);
        mcu_->queue_step(STEPPER_COREXY_B, interval, static_cast<uint16_t>(steps), 0);
    } else if (axis == 'Z' || axis == 'z') {
        /* Drive all 3 Z lead screws together */
        mcu_->set_step_dir(STEPPER_Z1, dir);
        mcu_->queue_step(STEPPER_Z1, interval, static_cast<uint16_t>(steps), 0);
        mcu_->set_step_dir(STEPPER_Z2, dir);
        mcu_->queue_step(STEPPER_Z2, interval, static_cast<uint16_t>(steps), 0);
        mcu_->set_step_dir(STEPPER_Z3, dir);
        mcu_->queue_step(STEPPER_Z3, interval, static_cast<uint16_t>(steps), 0);
    } else {
        mcu_->set_step_dir(channel, dir);
        mcu_->queue_step(channel, interval, static_cast<uint16_t>(steps), 0);
    }
}

void Engine::set_nozzle_temp(double temp_c) {
    if (heaters_[0]) {
        heaters_[0]->set_target(temp_c);
    }
}

void Engine::set_bed_temp(double temp_c) {
    if (heaters_[1]) {
        heaters_[1]->set_target(temp_c);
    }
}

void Engine::set_fan_speed(int fan, int percent) {
    spdlog::debug("Set fan {} speed: {}%", fan, percent);
    uint16_t duty = static_cast<uint16_t>(percent * 65535 / 100);
    uint8_t channel = static_cast<uint8_t>(PWM_FAN_PART + fan);
    mcu_->set_fan_pwm(channel, duty);
}

void Engine::set_valve_mask(uint8_t mask) {
    send_valve_state(mask & 0x0F);
}

void Engine::emergency_stop() {
    spdlog::error("EMERGENCY STOP");
    state_ = PrinterState::Error;

    /* Close all valves */
    send_valve_state(0);

    for (auto& h : heaters_) {
        if (h) h->kill();
    }

    mcu_->emergency_stop();
}

double Engine::progress() const {
    return 0.0;
}

double Engine::nozzle_temp() const {
    if (heaters_[0]) {
        return heaters_[0]->current_temp();
    }
    return 0.0;
}

double Engine::bed_temp() const {
    if (heaters_[1]) {
        return heaters_[1]->current_temp();
    }
    return 0.0;
}

} // namespace hydra
