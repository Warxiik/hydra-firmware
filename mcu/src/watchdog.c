#include "watchdog.h"
#include "config.h"
#include "gpio.h"
#include "hardware/watchdog.h"

void hydra_watchdog_init(void) {
    watchdog_enable(WATCHDOG_TIMEOUT_MS, true);
}

void hydra_watchdog_feed(void) {
    watchdog_update();
}
