#pragma once

#include <string>
#include <cstdint>

namespace hydra {

/**
 * Print state checkpoint for power-loss recovery.
 *
 * Periodically saved to eMMC during printing. On startup, the engine
 * checks for a checkpoint file and offers to resume.
 */
struct PrintCheckpoint {
    /* G-code file identification */
    std::string gcode_path;

    /* Per-nozzle stream position (line numbers) */
    uint32_t nozzle0_line = 0;
    uint32_t nozzle1_line = 0;

    /* Nozzle positions at checkpoint */
    double pos_x = 0, pos_y = 0, pos_z = 0;
    double e0 = 0, e1 = 0;

    /* Heater targets */
    double nozzle0_target = 0;
    double nozzle1_target = 0;
    double bed_target = 0;

    /* Fan speeds (0-255) */
    int fan0_speed = 0;
    int fan1_speed = 0;

    /* Print progress */
    uint32_t layer = 0;
    double progress_pct = 0;

    /* Timestamp (monotonic seconds since print start) */
    double elapsed_s = 0;
};

/**
 * Manages checkpoint save/load for print recovery.
 */
class CheckpointManager {
public:
    explicit CheckpointManager(const std::string& checkpoint_dir);

    /** Save a checkpoint to disk. */
    bool save(const PrintCheckpoint& cp);

    /** Load a checkpoint from disk. Returns false if none exists. */
    bool load(PrintCheckpoint& cp_out);

    /** Delete the checkpoint file (called on successful print completion). */
    void clear();

    /** Check if a checkpoint file exists. */
    bool exists() const;

    /** Get the checkpoint file path. */
    std::string checkpoint_path() const;

private:
    std::string dir_;
};

} // namespace hydra
