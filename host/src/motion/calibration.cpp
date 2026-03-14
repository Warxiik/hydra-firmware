#include "calibration.h"
#include <spdlog/spdlog.h>
#include <cmath>

namespace hydra::motion {

NozzleCalibration::NozzleCalibration(comms::McuProxy& mcu, const Config& config)
    : mcu_(mcu), config_(config) {}

void NozzleCalibration::start() {
    spdlog::info("Starting nozzle alignment calibration");
    phase_ = Phase::Homing;
    phase_ticks_ = 0;
    result_ = {};
}

void NozzleCalibration::queue_line(double x, double y, double e_rate,
                                    double speed, uint8_t extruder_channel) {
    /*
     * Simplified: compute step counts from position deltas.
     * In a full implementation, this would go through the planner.
     * For calibration, we queue direct moves at constant speed.
     */
    double dx = x, dy = y; /* These are deltas, not absolute */
    double distance = std::sqrt(dx * dx + dy * dy);
    if (distance < 0.001) return;

    double duration = distance / speed;
    double extrusion = e_rate * distance;

    /* CoreXY transform: A = X+Y, B = X-Y */
    double da = dx + dy;
    double db = dx - dy;

    int a_steps = static_cast<int>(std::abs(da) * config_.steps_per_mm_xy);
    int b_steps = static_cast<int>(std::abs(db) * config_.steps_per_mm_xy);
    int e_steps = static_cast<int>(std::abs(extrusion) * config_.steps_per_mm_e);

    auto queue_axis = [&](uint8_t channel, int steps, double delta, double spm) {
        if (steps == 0) return;
        mcu_.set_step_dir(channel, delta < 0 ? 1 : 0);
        uint32_t interval = static_cast<uint32_t>(
            duration * 1000000.0 / static_cast<double>(steps));
        if (interval < 10) interval = 10;
        mcu_.queue_step(channel, interval, static_cast<uint16_t>(steps), 0);
    };

    queue_axis(STEPPER_COREXY_A, a_steps, da, config_.steps_per_mm_xy);
    queue_axis(STEPPER_COREXY_B, b_steps, db, config_.steps_per_mm_xy);

    if (e_steps > 0) {
        mcu_.set_step_dir(extruder_channel, 0);
        uint32_t e_interval = static_cast<uint32_t>(
            duration * 1000000.0 / static_cast<double>(e_steps));
        if (e_interval < 10) e_interval = 10;
        mcu_.queue_step(extruder_channel, e_interval,
                        static_cast<uint16_t>(e_steps), 0);
    }
}

void NozzleCalibration::print_crosshair(uint8_t extruder_channel,
                                         double center_x, double center_y) {
    double half = CROSSHAIR_SIZE / 2.0;
    /*
     * E rate: extrusion per mm of travel.
     * cross_section = layer_height * line_width
     * filament_area = pi * (1.75/2)^2
     * e_rate = cross_section / filament_area
     */
    double cross_section = LAYER_HEIGHT * LINE_WIDTH;
    double filament_area = 3.14159 * 0.875 * 0.875;
    double e_rate = cross_section / filament_area;

    /* Move to start of horizontal line (travel) */
    queue_line(-half, 0, 0, CAL_SPEED * 3, extruder_channel);

    /* Horizontal line: left → right */
    queue_line(CROSSHAIR_SIZE, 0, e_rate, CAL_SPEED, extruder_channel);

    /* Move to start of vertical line */
    queue_line(-half, -half, 0, CAL_SPEED * 3, extruder_channel);

    /* Vertical line: bottom → top */
    queue_line(0, CROSSHAIR_SIZE, e_rate, CAL_SPEED, extruder_channel);

    /* Return to center */
    queue_line(0, -half, 0, CAL_SPEED * 3, extruder_channel);
}

bool NozzleCalibration::tick() {
    phase_ticks_++;

    switch (phase_) {
    case Phase::Homing:
        /*
         * In a real implementation, this would drive the HomingController.
         * For now, transition after a delay (homing handled by engine).
         */
        if (phase_ticks_ > 5000) {
            spdlog::info("Calibration: homing assumed complete, heating nozzle 0");
            phase_ = Phase::HeatingNozzle0;
            phase_ticks_ = 0;
        }
        return false;

    case Phase::HeatingNozzle0:
        /* Wait for temperature to stabilize (handled externally) */
        if (phase_ticks_ > 10000) {
            spdlog::info("Calibration: printing crosshair with nozzle 0");

            /* Move to calibration height */
            int z_steps = static_cast<int>(LAYER_HEIGHT * config_.steps_per_mm_z);
            mcu_.set_step_dir(STEPPER_Z, 0);
            mcu_.queue_step(STEPPER_Z, 500, static_cast<uint16_t>(z_steps), 0);

            /* Print crosshair with nozzle 0 */
            print_crosshair(STEPPER_EXTRUDER_0, CENTER_X, CENTER_Y);

            phase_ = Phase::PrintingCrosshair0;
            phase_ticks_ = 0;
        }
        return false;

    case Phase::PrintingCrosshair0:
        /* Wait for print to complete (queue drains) */
        if (mcu_.last_status().queue_depth[STEPPER_COREXY_A] == 0 &&
            phase_ticks_ > 500) {
            spdlog::info("Calibration: nozzle 0 crosshair done, heating nozzle 1");
            phase_ = Phase::HeatingNozzle1;
            phase_ticks_ = 0;
        }
        return false;

    case Phase::HeatingNozzle1:
        if (phase_ticks_ > 10000) {
            spdlog::info("Calibration: printing crosshair with nozzle 1");

            /* Print crosshair with nozzle 1 at same target position */
            /* The offset actuator should position nozzle 1 at the same
               XY as nozzle 0. Any misalignment will show as offset. */
            print_crosshair(STEPPER_EXTRUDER_1, CENTER_X, CENTER_Y);

            phase_ = Phase::PrintingCrosshair1;
            phase_ticks_ = 0;
        }
        return false;

    case Phase::PrintingCrosshair1:
        if (mcu_.last_status().queue_depth[STEPPER_COREXY_A] == 0 &&
            phase_ticks_ > 500) {
            spdlog::info("Calibration: both crosshairs printed — measure offset");
            spdlog::info("  Measure X/Y distance between nozzle 0 and nozzle 1 crosshairs");
            spdlog::info("  Submit via API: POST /api/calibration/submit {{x: ..., y: ...}}");
            phase_ = Phase::WaitingForMeasurement;
            phase_ticks_ = 0;
        }
        return false;

    case Phase::WaitingForMeasurement:
        /* Waiting for user to call submit_measurement() */
        return false;

    case Phase::Complete:
    case Phase::Failed:
        return true;

    default:
        return false;
    }
}

void NozzleCalibration::submit_measurement(double measured_x_mm, double measured_y_mm) {
    if (phase_ != Phase::WaitingForMeasurement) return;

    result_.offset_x_mm = measured_x_mm;
    result_.offset_y_mm = measured_y_mm;

    spdlog::info("Nozzle calibration result: X={:.3f}mm, Y={:.3f}mm",
                 measured_x_mm, measured_y_mm);
    spdlog::info("Apply this offset to the nozzle offset configuration");

    phase_ = Phase::Complete;
}

} // namespace hydra::motion
