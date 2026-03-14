#pragma once

#include "../core/engine.h"
#include <nlohmann/json.hpp>
#include <string>
#include <thread>
#include <atomic>
#include <mutex>
#include <vector>
#include <functional>

namespace hydra::web {

using json = nlohmann::json;

/**
 * Web server providing REST API + WebSocket for the React UI.
 *
 * REST endpoints:
 *   GET  /api/status          - Printer state, temps, progress
 *   POST /api/print/start     - Start print {file: "..."}
 *   POST /api/print/pause     - Pause
 *   POST /api/print/resume    - Resume
 *   POST /api/print/cancel    - Cancel
 *   POST /api/home            - Home axes {axes: "XYZ"}
 *   POST /api/jog             - Jog {axis, distance, speed}
 *   POST /api/temp/nozzle     - Set nozzle temp {nozzle, temp}
 *   POST /api/temp/bed        - Set bed temp {temp}
 *   POST /api/fan             - Set fan {fan, percent}
 *   POST /api/emergency_stop  - E-stop
 *   GET  /api/files           - List gcode files
 *
 * WebSocket:
 *   /ws  - Live status broadcast at ~10Hz
 *
 * Static files:
 *   /*   - Serves the React UI build (ui/dist/)
 */
class WebServer {
public:
    WebServer(Engine& engine, const std::string& static_dir, int port = 5000);
    ~WebServer();

    void start();
    void stop();
    bool is_running() const { return running_; }

private:
    void server_thread();
    void broadcast_thread();

    json build_status_json() const;
    std::string state_to_string(PrinterState state) const;

    Engine& engine_;
    std::string static_dir_;
    int port_;
    std::atomic<bool> running_{false};
    std::thread server_thread_;
    std::thread broadcast_thread_;

    /* WebSocket client tracking */
    struct WsClient;
    std::mutex clients_mutex_;
    std::vector<WsClient*> clients_;
};

} // namespace hydra::web
