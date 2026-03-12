#include "engine.h"
#include <spdlog/spdlog.h>
#include <thread>
#include <chrono>

namespace hydra {

Engine::Engine(const Config& config) : config_(config) {
    spdlog::info("Initializing print engine");
    /* TODO: Initialize MCU proxy, heaters, etc. */
}

Engine::~Engine() {
    running_ = false;
}

void Engine::run() {
    running_ = true;
    spdlog::info("Engine main loop started");

    while (running_) {
        control_loop_tick();
        /* ~1kHz control loop */
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

void Engine::control_loop_tick() {
    /* 1. Poll MCU for status (temperatures, endstops, queue depth) */
    /* 2. Run thermal PID, send heater PWM to MCU */
    /* 3. If printing: drain sync engine, feed planner, send steps */
    /* 4. Update state machine */
}

bool Engine::start_print(const std::string& gcode_base_path) {
    if (state_ != PrinterState::Idle) {
        spdlog::warn("Cannot start print: not idle (state={})", (int)state_.load());
        return false;
    }

    spdlog::info("Starting print: {}", gcode_base_path);
    /* TODO: Open dual gcode streams, create sync engine, begin heating */
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
    /* TODO: Flush queues, retract, lift, cool down */
    state_ = PrinterState::Idle;
}

void Engine::home_all() {
    spdlog::info("Homing all axes");
    state_ = PrinterState::Homing;
    /* TODO: Execute homing sequence */
}

void Engine::jog(char axis, double distance_mm, double speed_mm_s) {
    if (state_ != PrinterState::Idle) return;
    spdlog::debug("Jog {}={:.1f}mm @ {:.0f}mm/s", axis, distance_mm, speed_mm_s);
    /* TODO: Generate single move, send to planner */
}

void Engine::set_nozzle_temp(int nozzle, double temp_c) {
    spdlog::info("Set nozzle {} temp: {:.0f}C", nozzle, temp_c);
    /* TODO: Update heater target */
}

void Engine::set_bed_temp(double temp_c) {
    spdlog::info("Set bed temp: {:.0f}C", temp_c);
    /* TODO: Update heater target */
}

void Engine::set_fan_speed(int fan, int percent) {
    spdlog::debug("Set fan {} speed: {}%", fan, percent);
    /* TODO: Send PWM to MCU */
}

void Engine::emergency_stop() {
    spdlog::error("EMERGENCY STOP");
    state_ = PrinterState::Error;
    /* TODO: Send CMD_EMERGENCY_STOP to MCU */
}

double Engine::progress() const {
    /* TODO: Calculate from current layer / total layers */
    return 0.0;
}

} // namespace hydra
