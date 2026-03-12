#pragma once

#include "config.h"
#include "../gcode/stream.h"
#include "../gcode/sync_engine.h"
#include "../motion/planner.h"
#include "../thermal/heater.h"
#include "../comms/mcu_proxy.h"
#include <atomic>
#include <string>

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

private:
    void control_loop_tick();

    const Config& config_;
    std::atomic<PrinterState> state_{PrinterState::Idle};
    std::atomic<bool> running_{false};

    /* Subsystems — constructed lazily or in constructor */
    // std::unique_ptr<McuProxy> mcu_;
    // std::unique_ptr<GcodeStream> stream_;
    // std::unique_ptr<SyncEngine> sync_;
    // std::unique_ptr<Planner> planner_[2];
    // std::unique_ptr<HeaterManager> heaters_;
};

} // namespace hydra
