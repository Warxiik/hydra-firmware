#pragma once

#include "../comms/mcu_proxy.h"
#include <cstdint>
#include <string>
#include <array>

namespace hydra::drivers {

/**
 * TMC2209 register addresses.
 */
namespace tmc_reg {
    constexpr uint8_t GCONF       = 0x00;
    constexpr uint8_t GSTAT       = 0x01;
    constexpr uint8_t IOIN        = 0x06;
    constexpr uint8_t IHOLD_IRUN  = 0x10;
    constexpr uint8_t TPOWERDOWN  = 0x11;
    constexpr uint8_t TSTEP       = 0x12;
    constexpr uint8_t TPWMTHRS    = 0x13;
    constexpr uint8_t TCOOLTHRS   = 0x14;
    constexpr uint8_t SGTHRS      = 0x40;
    constexpr uint8_t SG_RESULT   = 0x41;
    constexpr uint8_t COOLCONF    = 0x42;
    constexpr uint8_t MSCNT       = 0x6A;
    constexpr uint8_t CHOPCONF    = 0x6C;
    constexpr uint8_t DRV_STATUS  = 0x6F;
    constexpr uint8_t PWMCONF     = 0x70;
}

/**
 * Per-driver configuration.
 */
struct TmcDriverConfig {
    uint8_t driver_id = 0;      /* Stepper channel (0-6) */
    uint16_t run_current_ma = 800;
    uint16_t hold_current_ma = 400;
    uint8_t microsteps = 16;    /* 1, 2, 4, 8, 16, 32, 64, 128, 256 */
    bool stealthchop = true;    /* true = StealthChop, false = SpreadCycle */
    uint8_t stall_threshold = 100; /* StallGuard threshold (0-255) */
    uint8_t hold_delay = 6;     /* IHOLDDELAY (0-15) */
    uint8_t toff = 3;           /* Off time (1-15, 0 = driver disabled) */
};

/**
 * TMC2209 driver configurator.
 * Applies configuration to all 7 TMC2209 drivers via the MCU proxy.
 */
class TmcConfig {
public:
    explicit TmcConfig(comms::McuProxy& mcu);

    /** Configure a single driver. */
    bool configure_driver(const TmcDriverConfig& cfg);

    /** Configure all drivers from an array of configs. */
    bool configure_all(const std::array<TmcDriverConfig, STEPPER_COUNT>& configs);

    /** Read driver status register. */
    bool read_status(uint8_t driver_id, uint32_t& status_out);

    /** Check if a driver reports errors (overtemp, short, open). */
    bool check_driver_ok(uint8_t driver_id);

    /** Enable/disable all stepper drivers via the global enable pin. */
    bool set_enabled(bool enabled);

    /**
     * Prepare a driver for sensorless homing (StallGuard4).
     * Switches to SpreadCycle, enables DIAG output, sets TCOOLTHRS
     * for the homing velocity, and sets SGTHRS threshold.
     *
     * @param driver_id  Stepper channel (0-6)
     * @param threshold  StallGuard threshold (0-255, higher = less sensitive)
     * @param homing_speed_mm_s  Homing speed to calculate TCOOLTHRS
     * @param steps_per_mm  Steps/mm for TCOOLTHRS calculation
     */
    bool enable_stallguard(uint8_t driver_id, uint8_t threshold,
                           double homing_speed_mm_s, double steps_per_mm);

    /**
     * Restore normal operating mode after sensorless homing.
     * Disables DIAG output, restores StealthChop if configured.
     */
    bool disable_stallguard(uint8_t driver_id, const TmcDriverConfig& cfg);

    /**
     * Read StallGuard result value (0-510, lower = more load).
     */
    bool read_stallguard(uint8_t driver_id, uint16_t& sg_result);

private:
    /** Map driver_id (0-6) to TMC bus + address. */
    struct BusAddr {
        uint8_t bus;
        uint8_t addr;
    };
    static BusAddr driver_bus_addr(uint8_t driver_id);

    /** Convert milliamps to IRUN/IHOLD register value (0-31). */
    static uint8_t current_to_cs(uint16_t current_ma, double r_sense = 0.11);

    /** Convert microstep count to MRES register field. */
    static uint8_t microsteps_to_mres(uint16_t microsteps);

    /** Build CHOPCONF register value. */
    static uint32_t build_chopconf(const TmcDriverConfig& cfg);

    /** Build IHOLD_IRUN register value. */
    static uint32_t build_ihold_irun(const TmcDriverConfig& cfg);

    /** Build GCONF register value. */
    static uint32_t build_gconf(const TmcDriverConfig& cfg);

    /** Build PWMCONF register value (StealthChop tuning). */
    static uint32_t build_pwmconf(const TmcDriverConfig& cfg);

    comms::McuProxy& mcu_;
};

} // namespace hydra::drivers
