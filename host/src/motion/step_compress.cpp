#include "step_compress.h"
#include <cmath>
#include <algorithm>

namespace hydra::motion {

std::vector<double> generate_step_times(int total_steps,
                                        int accel_steps, int cruise_steps, int decel_steps,
                                        double start_interval_us, double cruise_interval_us,
                                        double end_interval_us) {
    std::vector<double> times;
    times.reserve(total_steps);

    double t = 0.0;

    /* Acceleration phase: interval decreases linearly */
    for (int i = 0; i < accel_steps; i++) {
        double frac = (accel_steps > 1) ? static_cast<double>(i) / (accel_steps - 1) : 1.0;
        double interval = start_interval_us + frac * (cruise_interval_us - start_interval_us);
        t += interval;
        times.push_back(t);
    }

    /* Cruise phase: constant interval */
    for (int i = 0; i < cruise_steps; i++) {
        t += cruise_interval_us;
        times.push_back(t);
    }

    /* Deceleration phase: interval increases linearly */
    for (int i = 0; i < decel_steps; i++) {
        double frac = (decel_steps > 1) ? static_cast<double>(i) / (decel_steps - 1) : 1.0;
        double interval = cruise_interval_us + frac * (end_interval_us - cruise_interval_us);
        t += interval;
        times.push_back(t);
    }

    return times;
}

std::vector<StepSegment> compress_steps(const double* step_times, int num_steps,
                                        double max_error_us) {
    std::vector<StepSegment> segments;
    if (num_steps <= 0) return segments;

    /*
     * Greedy compression: try to fit as many steps as possible into
     * a single {interval, count, add} segment.
     *
     * The model: step[i] fires at base_time + interval + add * i
     * where interval adjusts per step: actual_interval[i] = interval + add * i
     *
     * We use linear regression on the intervals to find best (interval, add).
     */
    int i = 0;
    while (i < num_steps) {
        /* Try to extend the segment as far as possible */
        int best_count = 1;

        /* First interval */
        double first_time = step_times[i];
        double base_time = (i == 0) ? 0.0 : step_times[i - 1];
        double first_interval = first_time - base_time;

        if (i + 1 >= num_steps) {
            /* Single step remaining */
            StepSegment seg;
            seg.interval = static_cast<uint32_t>(std::round(first_interval));
            seg.count = 1;
            seg.add = 0;
            segments.push_back(seg);
            break;
        }

        /* Compute intervals for remaining steps */
        int max_count = std::min(num_steps - i, 65535);

        /* Try progressively larger segments */
        double sum_interval = first_interval;
        double sum_add = 0.0;

        for (int n = 2; n <= max_count; n++) {
            double t_prev = step_times[i + n - 2];
            double t_curr = step_times[i + n - 1];
            double interval_n = t_curr - t_prev;

            /* Linear fit: interval_k = interval_0 + add * k */
            double add_estimate = (interval_n - first_interval) / (n - 1);
            uint32_t interval_u32 = static_cast<uint32_t>(std::round(first_interval));
            int16_t add_i16 = static_cast<int16_t>(std::round(add_estimate));

            /* Check if all steps in this segment fit within error tolerance */
            bool fits = true;
            double predicted_time = base_time;
            for (int k = 0; k < n; k++) {
                double predicted_interval = static_cast<double>(interval_u32)
                                          + static_cast<double>(add_i16) * k;
                if (predicted_interval < 1.0) predicted_interval = 1.0;
                predicted_time += predicted_interval;

                double actual_time = step_times[i + k];
                if (std::abs(predicted_time - actual_time) > max_error_us) {
                    fits = false;
                    break;
                }
            }

            if (fits) {
                best_count = n;
            } else {
                break;
            }
        }

        /* Build the segment with best fit */
        double last_interval = step_times[i + best_count - 1]
                             - ((i + best_count >= 2) ? step_times[i + best_count - 2] : base_time);
        double add_val = (best_count > 1) ? (last_interval - first_interval) / (best_count - 1) : 0.0;

        StepSegment seg;
        seg.interval = static_cast<uint32_t>(std::round(first_interval));
        seg.count = static_cast<uint16_t>(best_count);
        seg.add = static_cast<int16_t>(std::round(add_val));

        if (seg.interval < 1) seg.interval = 1;
        segments.push_back(seg);

        i += best_count;
    }

    return segments;
}

} // namespace hydra::motion
