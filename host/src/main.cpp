#include "core/engine.h"
#include "core/config.h"
#include "version.h"
#include <spdlog/spdlog.h>
#include <iostream>
#include <csignal>

static hydra::Engine* g_engine = nullptr;

static void signal_handler(int sig) {
    spdlog::warn("Signal {} received, shutting down...", sig);
    if (g_engine) {
        g_engine->emergency_stop();
    }
}

int main(int argc, char* argv[]) {
    spdlog::set_level(spdlog::level::info);
    spdlog::info("Hydra Firmware v{}", HYDRA_VERSION_STRING);

    /* Load machine configuration */
    std::string config_path = (argc > 1) ? argv[1] : "/etc/hydra/hydra.toml";
    auto config = hydra::Config::load(config_path);
    if (!config) {
        spdlog::error("Failed to load config: {}", config_path);
        return 1;
    }

    /* Set up signal handlers */
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);

    /* Create and run the print engine */
    hydra::Engine engine(*config);
    g_engine = &engine;

    spdlog::info("Engine initialized, entering main loop");
    engine.run(); /* Blocks until shutdown */

    spdlog::info("Hydra shutdown complete");
    return 0;
}
