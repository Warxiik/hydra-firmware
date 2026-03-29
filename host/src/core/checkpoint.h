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

    /* Stream position (line number) */
    uint32_t line = 0;

    /* Nozzle position at checkpoint */
    double pos_x = 0, pos_y = 0, pos_z = 0;
    double e = 0;

    /* Heater targets */
    double manifold_target = 0;
    double bed_target = 0;

    /* Fan speed (0-255) */
    int fan_speed = 0;

    /* Valve state */
    uint8_t valve_mask = 0;

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
