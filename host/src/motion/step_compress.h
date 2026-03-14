#pragma once

#include <cstdint>
#include <vector>

namespace hydra::motion {

/**
 * Step compression: converts a velocity profile into compact
 * {interval, count, add} segments for the MCU step queue.
 *
 * Klipper-style step timing model:
 *   step[0] fires at: reference_clock + interval
 *   step[i] fires at: step[i-1] + interval + add * i
 *
 * This encodes constant velocity (add=0), acceleration (add<0),
 * and deceleration (add>0) as a single compact command.
 */

struct StepSegment {
    uint32_t interval;  /* Clock ticks for first step */
    uint16_t count;     /* Number of steps */
    int16_t  add;       /* Interval change per step */
};

/**
 * Compress a set of step times into segments.
 *
 * @param step_times    Absolute times (in µs) of each step
 * @param num_steps     Number of steps
 * @param max_error_us  Maximum timing error per step (µs)
 * @return              Compressed segments
 */
std::vector<StepSegment> compress_steps(const double* step_times, int num_steps,
                                        double max_error_us = 0.5);

/**
 * Generate step times for a trapezoidal velocity profile on a single axis.
 *
 * @param total_steps       Number of steps for this move
 * @param accel_steps       Steps during acceleration phase
 * @param cruise_steps      Steps during cruise phase
 * @param decel_steps       Steps during deceleration phase
 * @param start_interval_us Initial step interval (µs)
 * @param cruise_interval_us Cruise step interval (µs)
 * @param end_interval_us   Final step interval (µs)
 * @return                  Vector of absolute step times (µs from move start)
 */
std::vector<double> generate_step_times(int total_steps,
                                        int accel_steps, int cruise_steps, int decel_steps,
                                        double start_interval_us, double cruise_interval_us,
                                        double end_interval_us);

} // namespace hydra::motion
