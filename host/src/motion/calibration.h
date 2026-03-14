#pragma once

#include "../comms/mcu_proxy.h"
#include "../core/config.h"
#include "protocol_defs.h"
#include <functional>
#include <array>

namespace hydra::motion {

/**
 * Nozzle alignment calibration routine.
 *
 * Prints a calibration pattern and asks the user to measure the offset
 * between nozzle 0 and nozzle 1 marks. The measured offset is used to
 * update the firmware's nozzle offset compensation.
 *
 * Calibration procedure:
 *   1. Home all axes
 *   2. Heat nozzles to calibration temp (low, e.g. 190°C for PLA)
 *   3. Nozzle 0 prints a crosshair at a known position
 *   4. Nozzle 1 prints a crosshair at the same target position
 *   5. User measures the X/Y offset between the two crosshairs
 *   6. Offset is stored in config and applied to frame decomposer
 *
 * The result is the XY error between where nozzle 1 actually prints
 * vs where it should print, expressed as a correction vector.
 */
class NozzleCalibration {
public:
    enum class Phase {
        Idle,
        Homing,
        HeatingNozzle0,
        PrintingCrosshair0,
        HeatingNozzle1,
        PrintingCrosshair1,
        WaitingForMeasurement,
        Complete,
        Failed,
    };

    struct Result {
        double offset_x_mm = 0;  /* Correction to apply in X */
        double offset_y_mm = 0;  /* Correction to apply in Y */
    };

    NozzleCalibration(comms::McuProxy& mcu, const Config& config);

    /** Start the calibration sequence. */
    void start();

    /** Tick the state machine. */
    bool tick();

    /** Submit user's measured offset (called after measurement). */
    void submit_measurement(double measured_x_mm, double measured_y_mm);

    Phase phase() const { return phase_; }
    bool succeeded() const { return phase_ == Phase::Complete; }
    Result result() const { return result_; }

private:
    /** Generate G-code moves for a crosshair pattern. */
    void print_crosshair(uint8_t extruder_channel, double center_x, double center_y);

    /** Queue a single linear move. */
    void queue_line(double x, double y, double e_rate, double speed,
                    uint8_t extruder_channel);

    comms::McuProxy& mcu_;
    const Config& config_;
    Phase phase_ = Phase::Idle;
    Result result_;
    uint32_t phase_ticks_ = 0;

    /* Calibration parameters */
    static constexpr double CROSSHAIR_SIZE = 20.0;   /* mm — total crosshair length */
    static constexpr double LINE_WIDTH = 0.5;         /* mm — extrusion width */
    static constexpr double LAYER_HEIGHT = 0.2;       /* mm */
    static constexpr double CAL_SPEED = 20.0;         /* mm/s — slow for precision */
    static constexpr double CAL_TEMP = 190.0;         /* °C — PLA calibration temp */
    static constexpr double CENTER_X = 110.0;         /* mm — center of bed */
    static constexpr double CENTER_Y = 110.0;
};

} // namespace hydra::motion
