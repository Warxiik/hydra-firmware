#pragma once

#include "config.h"
#include "checkpoint.h"
#include "../gcode/stream.h"
#include "../gcode/sync_engine.h"
#include "../motion/planner.h"
#include "../motion/frame_decompose.h"
#include "../motion/homing.h"
#include "../thermal/heater.h"
#include "../comms/mcu_proxy.h"
#include "../drivers/tmc_config.h"
#include "../files/file_manager.h"
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
    FilamentChange,
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
    bool resume_print(); /* Resume from checkpoint */
    void pause();
    void resume();
    void cancel();

    /* Manual control */
    void home_all();
    void home_axes(bool x, bool y, bool z);
    void jog(char axis, double distance_mm, double speed_mm_s);
    void set_nozzle_temp(int nozzle, double temp_c);
    void set_bed_temp(double temp_c);
    void set_fan_speed(int fan, int percent);

    /* Filament change */
    void filament_change_confirm(); /* User confirms new filament loaded */

    /* Emergency */
    void emergency_stop();

    /* State queries */
    PrinterState state() const { return state_; }
    double progress() const;
    double nozzle_temp(int nozzle) const;
    double bed_temp() const;
    bool has_checkpoint() const;

    /* File manager access (for web server) */
    files::FileManager& file_manager() { return *file_manager_; }

private:
    void control_loop_tick(double dt_s);
    void tick_thermal(double dt_s);
    void tick_printing();
    void tick_homing();
    void tick_filament_change();
    void check_safety();
    void configure_drivers();
    void queue_decomposed_move(const motion::DecomposedMove& dm);
    void save_checkpoint();

    const Config& config_;
    std::atomic<PrinterState> state_{PrinterState::Idle};
    std::atomic<bool> running_{false};

    /* Subsystems */
    std::unique_ptr<comms::McuProxy> mcu_;
    std::unique_ptr<gcode::Stream> stream_;
    std::unique_ptr<gcode::SyncEngine> sync_;
    std::unique_ptr<motion::Planner> planner_[2];
    std::unique_ptr<motion::FrameDecomposer> decomposer_;
    std::unique_ptr<motion::HomingController> homing_ctrl_;
    std::unique_ptr<thermal::Heater> heaters_[3]; /* nozzle0, nozzle1, bed */
    std::unique_ptr<drivers::TmcConfig> tmc_config_;
    std::unique_ptr<files::FileManager> file_manager_;
    std::unique_ptr<CheckpointManager> checkpoint_mgr_;

    /* Timing */
    std::chrono::steady_clock::time_point last_tick_;
    std::chrono::steady_clock::time_point print_start_time_;
    uint32_t tick_count_ = 0;
    uint32_t clock_sync_interval_ = 100;
    uint32_t status_poll_interval_ = 10;
    uint32_t checkpoint_interval_ = 30000; /* Every 30s */

    /* Print tracking */
    std::string current_gcode_path_;
    uint32_t current_layer_ = 0;
};

} // namespace hydra
