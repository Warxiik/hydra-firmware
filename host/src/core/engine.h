#pragma once

#include "config.h"
#include "../gcode/stream.h"
#include "../gcode/sync_engine.h"
#include "../motion/planner.h"
#include "../thermal/heater.h"
#include "../comms/mcu_proxy.h"
#include <atomic>
#include <string>
#include <memory>
#include <chrono>

namespace hydra {

enum class PrinterState {
    Idle,
    Homing,
    Heating,
    Printing,
    Paused,
    Finishing,
    Error,
};

class Engine {
public:
    explicit Engine(const Config& config);
    ~Engine();

    /* Main loop — blocks until shutdown */
    void run();
    void shutdown();

    /* Print control */
    bool start_print(const std::string& gcode_base_path);
    void pause();
    void resume();
    void cancel();

    /* Manual control */
    void home_all();
    void jog(char axis, double distance_mm, double speed_mm_s);
    void set_nozzle_temp(int nozzle, double temp_c);
    void set_bed_temp(double temp_c);
    void set_fan_speed(int fan, int percent);

    /* Emergency */
    void emergency_stop();

    /* State queries */
    PrinterState state() const { return state_; }
    double progress() const;
    double nozzle_temp(int nozzle) const;
    double bed_temp() const;

private:
    void control_loop_tick(double dt_s);
    void tick_thermal(double dt_s);
    void tick_printing();
    void tick_homing();
    void check_safety();

    const Config& config_;
    std::atomic<PrinterState> state_{PrinterState::Idle};
    std::atomic<bool> running_{false};

    /* Subsystems */
    std::unique_ptr<comms::McuProxy> mcu_;
    std::unique_ptr<gcode::Stream> stream_;
    std::unique_ptr<gcode::SyncEngine> sync_;
    std::unique_ptr<motion::Planner> planner_[2];
    std::unique_ptr<thermal::Heater> heaters_[3]; /* nozzle0, nozzle1, bed */

    /* Timing */
    std::chrono::steady_clock::time_point last_tick_;
    uint32_t tick_count_ = 0;
    uint32_t clock_sync_interval_ = 100; /* Every 100 ticks (~100ms) */
    uint32_t status_poll_interval_ = 10; /* Every 10 ticks (~10ms) */
};

} // namespace hydra
