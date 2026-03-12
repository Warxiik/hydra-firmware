#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "hardware/watchdog.h"

#include "pins.h"
#include "config.h"
#include "stepper.h"
#include "transport.h"
#include "protocol.h"
#include "adc.h"
#include "gpio.h"
#include "watchdog.h"

/*
 * RP2040 dual-core architecture:
 *
 * Core 0 — Communication & Control
 *   - SPI slave: receive commands from CM5 host
 *   - Protocol decode & dispatch
 *   - TMC2209 UART configuration
 *   - ADC thermistor reading (round-robin)
 *   - Watchdog feeding
 *   - Status report transmission
 *
 * Core 1 — Step Generation
 *   - Manage PIO state machines (shift-register step output)
 *   - Drain step queues, feed step data to PIO
 *   - Endstop monitoring during homing
 */

static void core1_entry(void) {
    stepper_core1_init();

    while (true) {
        stepper_core1_loop();
    }
}

int main(void) {
    /* System init */
    stdio_init_all();

    /* Initialize subsystems */
    hydra_gpio_init();
    hydra_adc_init();
    stepper_init();
    transport_init();
    protocol_init();
    hydra_watchdog_init();

    /* Launch Core 1 for step generation */
    multicore_launch_core1(core1_entry);

    /* Core 0: communication & control loop */
    while (true) {
        transport_poll();        /* Check SPI for incoming frames     */
        protocol_process();      /* Decode and dispatch commands      */
        hydra_adc_poll();        /* Read thermistors (round-robin)    */
        hydra_watchdog_feed();   /* Feed watchdog if host is alive    */
        protocol_send_status();  /* Periodic status report to host    */
    }
}
