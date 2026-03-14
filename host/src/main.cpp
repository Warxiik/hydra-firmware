#include "core/engine.h"
#include "core/config.h"
#include "web/server.h"
#include "version.h"
#include <spdlog/spdlog.h>
#include <iostream>
#include <csignal>
#include <thread>

static hydra::Engine* g_engine = nullptr;
static hydra::web::WebServer* g_web = nullptr;

static void signal_handler(int sig) {
    spdlog::warn("Signal {} received, shutting down...", sig);
    if (g_web) g_web->stop();
    if (g_engine) g_engine->shutdown();
}

int main(int argc, char* argv[]) {
    spdlog::set_level(spdlog::level::info);
    spdlog::info("Hydra Firmware v{}", HYDRA_VERSION_STRING);

    /* Load machine configuration */
    std::string config_path = (argc > 1) ? argv[1] : "/etc/hydra/hydra.toml";
    auto config = hydra::Config::load(config_path);
    if (!config) {
        spdlog::warn("Config not found at {}, using defaults", config_path);
        config = hydra::Config{};
    }

    /* Set up signal handlers */
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);

    /* Create the print engine */
    hydra::Engine engine(*config);
    g_engine = &engine;

    /* Create and start web server */
    std::string ui_dist = "../ui/dist"; /* Relative to working dir */
    if (argc > 2) ui_dist = argv[2];

    hydra::web::WebServer web(engine, ui_dist, 5000);
    g_web = &web;
    web.start();

    spdlog::info("Web UI: http://localhost:5000");
    spdlog::info("Engine initialized, entering main loop");

    engine.run(); /* Blocks until shutdown */

    web.stop();
    spdlog::info("Hydra shutdown complete");
    return 0;
}
