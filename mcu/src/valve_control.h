#ifndef HYDRA_MCU_VALVE_CONTROL_H
#define HYDRA_MCU_VALVE_CONTROL_H

#include <stdint.h>

/**
 * Solenoid needle valve control for multi-nozzle array.
 *
 * 4 nozzles controlled by individual solenoid valves via MOSFET drivers.
 * Each valve GPIO drives an IRLZ34N gate → solenoid → push rod → needle.
 * HIGH = valve open (solenoid energized, needle retracted).
 * LOW  = valve closed (spring returns needle to seat).
 */

/* Initialize valve GPIO pins as outputs (all closed) */
void valve_control_init(void);

/* Set valve state from bitmask (bit N = nozzle N) */
void valve_control_set(uint8_t mask);

/* Get current valve bitmask */
uint8_t valve_control_get(void);

/* Close all valves (emergency / shutdown) */
void valve_control_close_all(void);

#endif /* HYDRA_MCU_VALVE_CONTROL_H */
