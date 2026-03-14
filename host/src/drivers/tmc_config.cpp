#include "tmc_config.h"
#include <spdlog/spdlog.h>
#include <cmath>
#include <thread>

namespace hydra::drivers {

TmcConfig::TmcConfig(comms::McuProxy& mcu) : mcu_(mcu) {}

TmcConfig::BusAddr TmcConfig::driver_bus_addr(uint8_t driver_id) {
    /*
     * Bus 0: drivers 0-3 (CoreXY A, CoreXY B, Offset X, Offset Y)
     *   Address on bus = driver_id (0-3)
     *
     * Bus 1: drivers 4-6 (Z, Extruder 0, Extruder 1)
     *   Address on bus = driver_id - 4 (0-2)
     */
    if (driver_id <= 3) {
        return {0, driver_id};
    } else {
        return {1, static_cast<uint8_t>(driver_id - 4)};
    }
}

uint8_t TmcConfig::current_to_cs(uint16_t current_ma, double r_sense) {
    /*
     * TMC2209 current formula:
     *   I_rms = (CS + 1) / 32 * V_fs / (sqrt(2) * R_sense)
     *
     * Where V_fs = 0.325V for TMC2209, R_sense = 0.11 ohm typical.
     *
     * Solving for CS:
     *   CS = (I_rms * 32 * sqrt(2) * R_sense / 0.325) - 1
     */
    double i_rms = current_ma / 1000.0;
    double cs = (i_rms * 32.0 * std::sqrt(2.0) * r_sense / 0.325) - 1.0;
    int cs_int = static_cast<int>(std::round(cs));
    if (cs_int < 0) cs_int = 0;
    if (cs_int > 31) cs_int = 31;
    return static_cast<uint8_t>(cs_int);
}

uint8_t TmcConfig::microsteps_to_mres(uint16_t microsteps) {
    /*
     * MRES encoding in CHOPCONF[27:24]:
     *   0 = 256, 1 = 128, 2 = 64, 3 = 32, 4 = 16,
     *   5 = 8, 6 = 4, 7 = 2, 8 = fullstep
     */
    switch (microsteps) {
    case 1:   return 8;
    case 2:   return 7;
    case 4:   return 6;
    case 8:   return 5;
    case 16:  return 4;
    case 32:  return 3;
    case 64:  return 2;
    case 128: return 1;
    case 256: return 0;
    default:  return 4; /* Default 16 microsteps */
    }
}

uint32_t TmcConfig::build_chopconf(const TmcDriverConfig& cfg) {
    uint32_t reg = 0;
    reg |= (cfg.toff & 0x0F);                                  /* TOFF [3:0] */
    reg |= (4u << 4);                                          /* HSTRT [6:4] = 4 */
    reg |= (1u << 7);                                          /* HEND [10:7] = 1 */
    reg |= (static_cast<uint32_t>(microsteps_to_mres(cfg.microsteps)) << 24); /* MRES [27:24] */
    reg |= (1u << 17);                                         /* vsense = 1 (low sense resistor range) */
    return reg;
}

uint32_t TmcConfig::build_ihold_irun(const TmcDriverConfig& cfg) {
    uint32_t reg = 0;
    uint8_t ihold = current_to_cs(cfg.hold_current_ma);
    uint8_t irun = current_to_cs(cfg.run_current_ma);

    reg |= (ihold & 0x1F);                             /* IHOLD [4:0] */
    reg |= (static_cast<uint32_t>(irun & 0x1F) << 8);  /* IRUN [12:8] */
    reg |= (static_cast<uint32_t>(cfg.hold_delay & 0x0F) << 16); /* IHOLDDELAY [19:16] */
    return reg;
}

uint32_t TmcConfig::build_gconf(const TmcDriverConfig& cfg) {
    uint32_t reg = 0;
    reg |= (1u << 0);  /* I_scale_analog = 1 (use internal reference) */
    reg |= (1u << 1);  /* internal_Rsense = 0 (external sense resistors) */
    if (!cfg.stealthchop) {
        reg |= (1u << 2); /* en_spreadCycle = 1 */
    }
    reg |= (1u << 6);  /* multistep_filt = 1 */
    return reg;
}

uint32_t TmcConfig::build_pwmconf(const TmcDriverConfig& /* cfg */) {
    /*
     * PWMCONF defaults tuned for StealthChop:
     * PWM_OFS = 36, PWM_GRAD = 14, pwm_freq = 01 (2/1024 f_clk),
     * pwm_autoscale = 1, pwm_autograd = 1
     */
    uint32_t reg = 0;
    reg |= 36;                  /* PWM_OFS [7:0] */
    reg |= (14u << 8);         /* PWM_GRAD [15:8] */
    reg |= (1u << 16);         /* pwm_freq [17:16] = 01 */
    reg |= (1u << 18);         /* pwm_autoscale = 1 */
    reg |= (1u << 19);         /* pwm_autograd = 1 */
    reg |= (1u << 20);         /* freewheel [21:20] = 01 (freewheeling) */
    return reg;
}

bool TmcConfig::configure_driver(const TmcDriverConfig& cfg) {
    spdlog::info("Configuring TMC2209 driver {} ({}mA run, {}mA hold, {} microsteps, {})",
                 cfg.driver_id, cfg.run_current_ma, cfg.hold_current_ma,
                 cfg.microsteps, cfg.stealthchop ? "StealthChop" : "SpreadCycle");

    auto [bus, addr] = driver_bus_addr(cfg.driver_id);

    /* Clear GSTAT (write 1 to clear error flags) */
    if (!mcu_.tmc_write(cfg.driver_id, tmc_reg::GSTAT, 0x07)) {
        spdlog::warn("TMC{} failed to clear GSTAT", cfg.driver_id);
    }

    /* Small delay between register writes to avoid bus contention */
    auto reg_delay = [](){ std::this_thread::sleep_for(std::chrono::milliseconds(2)); };

    /* Set GCONF (StealthChop vs SpreadCycle mode) */
    if (!mcu_.tmc_write(cfg.driver_id, tmc_reg::GCONF, build_gconf(cfg))) {
        spdlog::error("TMC{} failed to write GCONF", cfg.driver_id);
        return false;
    }
    reg_delay();

    /* Set CHOPCONF (microstepping, chopper parameters) */
    if (!mcu_.tmc_write(cfg.driver_id, tmc_reg::CHOPCONF, build_chopconf(cfg))) {
        spdlog::error("TMC{} failed to write CHOPCONF", cfg.driver_id);
        return false;
    }
    reg_delay();

    /* Set IHOLD_IRUN (run/hold current) */
    if (!mcu_.tmc_write(cfg.driver_id, tmc_reg::IHOLD_IRUN, build_ihold_irun(cfg))) {
        spdlog::error("TMC{} failed to write IHOLD_IRUN", cfg.driver_id);
        return false;
    }
    reg_delay();

    /* Set TPOWERDOWN (delay before hold current kicks in: ~2 seconds) */
    if (!mcu_.tmc_write(cfg.driver_id, tmc_reg::TPOWERDOWN, 20)) {
        spdlog::warn("TMC{} failed to write TPOWERDOWN", cfg.driver_id);
    }
    reg_delay();

    /* Set PWMCONF (StealthChop auto-tuning) */
    if (!mcu_.tmc_write(cfg.driver_id, tmc_reg::PWMCONF, build_pwmconf(cfg))) {
        spdlog::warn("TMC{} failed to write PWMCONF", cfg.driver_id);
    }
    reg_delay();

    /* Set TPWMTHRS (velocity threshold for StealthChop → SpreadCycle transition) */
    /* Value in clock cycles; lower = higher speed threshold */
    /* ~200 corresponds to roughly 50mm/s at 16 microsteps with 80 steps/mm */
    if (!mcu_.tmc_write(cfg.driver_id, tmc_reg::TPWMTHRS, 200)) {
        spdlog::warn("TMC{} failed to write TPWMTHRS", cfg.driver_id);
    }
    reg_delay();

    /* Set StallGuard threshold (for sensorless homing) */
    if (!mcu_.tmc_write(cfg.driver_id, tmc_reg::SGTHRS, cfg.stall_threshold)) {
        spdlog::warn("TMC{} failed to write SGTHRS", cfg.driver_id);
    }

    spdlog::info("TMC{} configured successfully", cfg.driver_id);
    return true;
}

bool TmcConfig::configure_all(const std::array<TmcDriverConfig, STEPPER_COUNT>& configs) {
    bool all_ok = true;
    for (const auto& cfg : configs) {
        if (!configure_driver(cfg)) {
            all_ok = false;
        }
    }
    return all_ok;
}

bool TmcConfig::read_status(uint8_t driver_id, uint32_t& status_out) {
    auto val = mcu_.tmc_read(driver_id, tmc_reg::DRV_STATUS);
    if (!val) return false;
    status_out = *val;
    return true;
}

bool TmcConfig::check_driver_ok(uint8_t driver_id) {
    uint32_t status;
    if (!read_status(driver_id, status)) {
        spdlog::warn("TMC{}: could not read DRV_STATUS", driver_id);
        return false;
    }

    bool ok = true;

    /* DRV_STATUS bit fields: */
    if (status & (1u << 1)) { /* ot: overtemperature */
        spdlog::error("TMC{}: overtemperature shutdown!", driver_id);
        ok = false;
    }
    if (status & (1u << 2)) { /* otpw: overtemperature warning */
        spdlog::warn("TMC{}: overtemperature warning", driver_id);
    }
    if (status & (1u << 3) || status & (1u << 4)) { /* s2ga/s2gb: short to ground */
        spdlog::error("TMC{}: short to ground detected!", driver_id);
        ok = false;
    }
    if (status & (1u << 5) || status & (1u << 6)) { /* ola/olb: open load */
        spdlog::warn("TMC{}: open load detected (motor disconnected?)", driver_id);
    }
    if (status & (1u << 0)) { /* stst: standstill */
        /* Not an error, just informational */
    }

    return ok;
}

bool TmcConfig::set_enabled(bool enabled) {
    /* The stepper enable pin is active low, directly controlled as a digital output */
    return mcu_.tmc_write(0, tmc_reg::GCONF, enabled ? 0 : 1);
}

} // namespace hydra::drivers
