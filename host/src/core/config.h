#pragma once

#include <string>
#include <optional>

namespace hydra {

struct Config {
    /* SPI connection to MCU */
    std::string spi_device = "/dev/spidev0.0";
    int spi_speed_hz = 4000000;

    /* Build volume (mm) */
    double bed_x = 220.0;
    double bed_y = 220.0;
    double bed_z = 250.0;

    /* Motion limits */
    double max_velocity = 300.0;        /* mm/s */
    double max_acceleration = 3000.0;   /* mm/s^2 */
    double max_z_velocity = 10.0;
    double max_z_acceleration = 100.0;
    double junction_deviation = 0.05;   /* mm */

    /* Nozzle offset actuator range (mm from center) */
    double offset_range_x = 30.0;
    double offset_range_y = 30.0;

    /* Stepper steps/mm */
    double steps_per_mm_xy = 80.0;    /* CoreXY A/B */
    double steps_per_mm_z = 400.0;
    double steps_per_mm_e = 93.0;     /* Extruder */
    double steps_per_mm_offset = 40.0; /* Nozzle offset actuators */

    /* Thermal PID */
    struct PID {
        double kp = 20.0, ki = 1.0, kd = 100.0;
    };
    PID nozzle_pid;
    PID bed_pid{10.0, 0.5, 200.0};

    /* File paths */
    std::string gcode_dir = "/home/hydra/gcodes";
    std::string log_dir = "/var/log/hydra";

    static std::optional<Config> load(const std::string& path);
};

} // namespace hydra
