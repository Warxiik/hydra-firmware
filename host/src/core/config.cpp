#include "config.h"
#include <spdlog/spdlog.h>
#include <toml++/toml.hpp>
#include <fstream>

namespace hydra {

std::optional<Config> Config::load(const std::string& path) {
    try {
        auto tbl = toml::parse_file(path);
        Config cfg;

        /* SPI */
        cfg.spi_device = tbl["mcu"]["spi_device"].value_or(cfg.spi_device);
        cfg.spi_speed_hz = tbl["mcu"]["spi_speed_hz"].value_or(cfg.spi_speed_hz);

        /* Build volume */
        cfg.bed_x = tbl["printer"]["bed_x"].value_or(cfg.bed_x);
        cfg.bed_y = tbl["printer"]["bed_y"].value_or(cfg.bed_y);
        cfg.bed_z = tbl["printer"]["bed_z"].value_or(cfg.bed_z);

        /* Motion */
        cfg.max_velocity = tbl["motion"]["max_velocity"].value_or(cfg.max_velocity);
        cfg.max_acceleration = tbl["motion"]["max_acceleration"].value_or(cfg.max_acceleration);
        cfg.max_z_velocity = tbl["motion"]["max_z_velocity"].value_or(cfg.max_z_velocity);
        cfg.junction_deviation = tbl["motion"]["junction_deviation"].value_or(cfg.junction_deviation);

        /* Steps/mm */
        cfg.steps_per_mm_xy = tbl["stepper"]["steps_per_mm_xy"].value_or(cfg.steps_per_mm_xy);
        cfg.steps_per_mm_z = tbl["stepper"]["steps_per_mm_z"].value_or(cfg.steps_per_mm_z);
        cfg.steps_per_mm_e = tbl["stepper"]["steps_per_mm_e"].value_or(cfg.steps_per_mm_e);

        /* Valve array */
        cfg.valve_count = tbl["valve"]["count"].value_or(cfg.valve_count);
        cfg.nozzle_spacing = tbl["valve"]["nozzle_spacing"].value_or(cfg.nozzle_spacing);
        cfg.valve_delay_ms = tbl["valve"]["delay_ms"].value_or(cfg.valve_delay_ms);

        /* Thermal PID */
        if (auto pid = tbl["thermal"]["nozzle_pid"]; pid.is_table()) {
            cfg.nozzle_pid.kp = pid["kp"].value_or(cfg.nozzle_pid.kp);
            cfg.nozzle_pid.ki = pid["ki"].value_or(cfg.nozzle_pid.ki);
            cfg.nozzle_pid.kd = pid["kd"].value_or(cfg.nozzle_pid.kd);
        }

        /* Paths */
        cfg.gcode_dir = tbl["paths"]["gcode_dir"].value_or(cfg.gcode_dir);

        spdlog::info("Config loaded: {}x{}x{}mm, {:.0f}mm/s max, {} valves",
                     cfg.bed_x, cfg.bed_y, cfg.bed_z, cfg.max_velocity, cfg.valve_count);
        return cfg;
    } catch (const toml::parse_error& err) {
        spdlog::error("Config parse error: {}", err.what());
        return std::nullopt;
    } catch (const std::exception& err) {
        spdlog::error("Config load error: {}", err.what());
        return std::nullopt;
    }
}

} // namespace hydra
