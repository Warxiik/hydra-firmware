#pragma once

#include <string>
#include <optional>
#include <cstdint>

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

    /* Homing */
    struct Homing {
        double fast_speed_xy = 50.0;    /* mm/s — fast approach */
        double slow_speed_xy = 5.0;     /* mm/s — slow probe */
        double fast_speed_z = 10.0;
        double slow_speed_z = 2.0;
        double backoff_mm = 5.0;        /* Distance to back off after endstop hit */
        double max_travel_xy = 250.0;   /* Max distance to travel before giving up */
        double max_travel_z = 260.0;
    };
    Homing homing;

    /* TMC2209 stepper driver settings */
    struct TMC {
        uint16_t xy_run_current_ma = 800;
        uint16_t xy_hold_current_ma = 400;
        uint16_t z_run_current_ma = 800;
        uint16_t z_hold_current_ma = 400;
        uint16_t e_run_current_ma = 600;
        uint16_t e_hold_current_ma = 300;
        uint16_t offset_run_current_ma = 600;
        uint16_t offset_hold_current_ma = 300;
        uint8_t microsteps = 16;
        bool stealthchop = true;
    };
    TMC tmc;

    /* File paths */
    std::string gcode_dir = "/home/hydra/gcodes";
    std::string log_dir = "/var/log/hydra";

    static std::optional<Config> load(const std::string& path);
};

} // namespace hydra
