#include "server.h"
#include <spdlog/spdlog.h>
#include <crow.h>
#include <fstream>
#include <filesystem>

namespace fs = std::filesystem;

namespace hydra::web {

struct WebServer::WsClient {
    crow::websocket::connection* conn;
};

WebServer::WebServer(Engine& engine, const std::string& static_dir, int port)
    : engine_(engine), static_dir_(static_dir), port_(port) {}

WebServer::~WebServer() {
    stop();
}

std::string WebServer::state_to_string(PrinterState state) const {
    switch (state) {
    case PrinterState::Idle:      return "idle";
    case PrinterState::Homing:    return "homing";
    case PrinterState::Heating:   return "heating";
    case PrinterState::Printing:  return "printing";
    case PrinterState::Paused:    return "paused";
    case PrinterState::Finishing:      return "finishing";
    case PrinterState::FilamentChange: return "filament_change";
    case PrinterState::Error:          return "error";
    default:                           return "unknown";
    }
}

json WebServer::build_status_json() const {
    json j;
    j["state"] = state_to_string(engine_.state());
    j["nozzle0Temp"] = engine_.nozzle_temp(0);
    j["nozzle0Target"] = 0; /* TODO: expose targets from engine */
    j["nozzle1Temp"] = engine_.nozzle_temp(1);
    j["nozzle1Target"] = 0;
    j["bedTemp"] = engine_.bed_temp();
    j["bedTarget"] = 0;
    j["progress"] = engine_.progress();
    j["currentLayer"] = 0;
    j["totalLayers"] = 0;
    j["elapsedSeconds"] = 0;
    j["remainingSeconds"] = 0;
    j["fileName"] = "";
    j["speedOverride"] = 100;
    j["flowOverride"] = 100;
    j["fanSpeed"] = 0;
    j["wifiConnected"] = true;
    j["wifiSSID"] = "";
    j["ipAddress"] = "";
    return j;
}

void WebServer::start() {
    if (running_) return;
    running_ = true;
    server_thread_ = std::thread(&WebServer::server_thread, this);
    broadcast_thread_ = std::thread(&WebServer::broadcast_thread, this);
    spdlog::info("Web server starting on port {}", port_);
}

void WebServer::stop() {
    running_ = false;
    if (broadcast_thread_.joinable()) broadcast_thread_.join();
    if (server_thread_.joinable()) server_thread_.join();
}

void WebServer::server_thread() {
    crow::SimpleApp app;

    /* ── REST API ───────────────────────────────────────────────── */

    CROW_ROUTE(app, "/api/status").methods(crow::HTTPMethod::GET)(
        [this]() {
            return crow::response(build_status_json().dump());
        });

    CROW_ROUTE(app, "/api/print/start").methods(crow::HTTPMethod::POST)(
        [this](const crow::request& req) {
            auto body = json::parse(req.body, nullptr, false);
            if (body.is_discarded() || !body.contains("file"))
                return crow::response(400, R"({"error":"missing file"})");
            bool ok = engine_.start_print(body["file"].get<std::string>());
            return crow::response(ok ? 200 : 409, ok ? R"({"ok":true})" : R"({"error":"not idle"})");
        });

    CROW_ROUTE(app, "/api/print/pause").methods(crow::HTTPMethod::POST)(
        [this]() { engine_.pause(); return crow::response(R"({"ok":true})"); });

    CROW_ROUTE(app, "/api/print/resume").methods(crow::HTTPMethod::POST)(
        [this]() { engine_.resume(); return crow::response(R"({"ok":true})"); });

    CROW_ROUTE(app, "/api/print/cancel").methods(crow::HTTPMethod::POST)(
        [this]() { engine_.cancel(); return crow::response(R"({"ok":true})"); });

    CROW_ROUTE(app, "/api/print/resume_checkpoint").methods(crow::HTTPMethod::POST)(
        [this]() {
            bool ok = engine_.resume_print();
            return crow::response(ok ? 200 : 404,
                ok ? R"({"ok":true})" : R"({"error":"no checkpoint"})");
        });

    CROW_ROUTE(app, "/api/print/has_checkpoint").methods(crow::HTTPMethod::GET)(
        [this]() {
            json j;
            j["hasCheckpoint"] = engine_.has_checkpoint();
            return crow::response(j.dump());
        });

    CROW_ROUTE(app, "/api/filament_change/confirm").methods(crow::HTTPMethod::POST)(
        [this]() {
            engine_.filament_change_confirm();
            return crow::response(R"({"ok":true})");
        });

    CROW_ROUTE(app, "/api/home").methods(crow::HTTPMethod::POST)(
        [this]() { engine_.home_all(); return crow::response(R"({"ok":true})"); });

    CROW_ROUTE(app, "/api/jog").methods(crow::HTTPMethod::POST)(
        [this](const crow::request& req) {
            auto body = json::parse(req.body, nullptr, false);
            if (body.is_discarded()) return crow::response(400);
            char axis = body.value("axis", "X")[0];
            double distance = body.value("distance", 0.0);
            double speed = body.value("speed", 50.0);
            engine_.jog(axis, distance, speed);
            return crow::response(R"({"ok":true})");
        });

    CROW_ROUTE(app, "/api/temp/nozzle").methods(crow::HTTPMethod::POST)(
        [this](const crow::request& req) {
            auto body = json::parse(req.body, nullptr, false);
            if (body.is_discarded()) return crow::response(400);
            int nozzle = body.value("nozzle", 0);
            double temp = body.value("temp", 0.0);
            engine_.set_nozzle_temp(nozzle, temp);
            return crow::response(R"({"ok":true})");
        });

    CROW_ROUTE(app, "/api/temp/bed").methods(crow::HTTPMethod::POST)(
        [this](const crow::request& req) {
            auto body = json::parse(req.body, nullptr, false);
            if (body.is_discarded()) return crow::response(400);
            double temp = body.value("temp", 0.0);
            engine_.set_bed_temp(temp);
            return crow::response(R"({"ok":true})");
        });

    CROW_ROUTE(app, "/api/fan").methods(crow::HTTPMethod::POST)(
        [this](const crow::request& req) {
            auto body = json::parse(req.body, nullptr, false);
            if (body.is_discarded()) return crow::response(400);
            int fan = body.value("fan", 0);
            int percent = body.value("percent", 0);
            engine_.set_fan_speed(fan, percent);
            return crow::response(R"({"ok":true})");
        });

    CROW_ROUTE(app, "/api/emergency_stop").methods(crow::HTTPMethod::POST)(
        [this]() { engine_.emergency_stop(); return crow::response(R"({"ok":true})"); });

    CROW_ROUTE(app, "/api/files").methods(crow::HTTPMethod::GET)(
        [this]() {
            json files = json::array();
            for (const auto& f : engine_.file_manager().list_files()) {
                json fj;
                fj["name"] = f.name;
                fj["size"] = f.size_bytes;
                fj["modified"] = f.modified;
                fj["isDual"] = f.is_dual;
                files.push_back(fj);
            }
            return crow::response(files.dump());
        });

    CROW_ROUTE(app, "/api/files/upload").methods(crow::HTTPMethod::POST)(
        [this](const crow::request& req) {
            auto body = json::parse(req.body, nullptr, false);
            if (body.is_discarded() || !body.contains("name") || !body.contains("data"))
                return crow::response(400, R"({"error":"missing name or data"})");

            std::string name = body["name"].get<std::string>();
            std::string data = body["data"].get<std::string>();

            bool ok = engine_.file_manager().save_file(name, data);
            return crow::response(ok ? 200 : 500,
                ok ? R"({"ok":true})" : R"({"error":"save failed"})");
        });

    CROW_ROUTE(app, "/api/files/delete").methods(crow::HTTPMethod::POST)(
        [this](const crow::request& req) {
            auto body = json::parse(req.body, nullptr, false);
            if (body.is_discarded() || !body.contains("name"))
                return crow::response(400, R"({"error":"missing name"})");

            std::string name = body["name"].get<std::string>();
            bool ok = engine_.file_manager().delete_file(name);
            return crow::response(ok ? 200 : 404,
                ok ? R"({"ok":true})" : R"({"error":"not found"})");
        });

    CROW_ROUTE(app, "/api/files/disk").methods(crow::HTTPMethod::GET)(
        [this]() {
            auto space = engine_.file_manager().disk_space();
            json j;
            j["total"] = space.total;
            j["free"] = space.free;
            return crow::response(j.dump());
        });

    /* ── WebSocket ──────────────────────────────────────────────── */

    CROW_WEBSOCKET_ROUTE(app, "/ws")
        .onopen([this](crow::websocket::connection& conn) {
            std::lock_guard lock(clients_mutex_);
            auto* client = new WsClient{&conn};
            clients_.push_back(client);
            spdlog::debug("WebSocket client connected (total: {})", clients_.size());
        })
        .onclose([this](crow::websocket::connection& conn, const std::string&, uint16_t) {
            std::lock_guard lock(clients_mutex_);
            clients_.erase(
                std::remove_if(clients_.begin(), clients_.end(),
                    [&conn](WsClient* c) {
                        if (c->conn == &conn) { delete c; return true; }
                        return false;
                    }),
                clients_.end());
            spdlog::debug("WebSocket client disconnected (total: {})", clients_.size());
        })
        .onmessage([this](crow::websocket::connection&, const std::string& data, bool) {
            /* Handle commands from the UI via WebSocket */
            auto msg = json::parse(data, nullptr, false);
            if (msg.is_discarded()) return;

            std::string type = msg.value("type", "");

            if (type == "start_print")
                engine_.start_print(msg.value("fileName", ""));
            else if (type == "pause")
                engine_.pause();
            else if (type == "resume")
                engine_.resume();
            else if (type == "cancel")
                engine_.cancel();
            else if (type == "home")
                engine_.home_all();
            else if (type == "jog") {
                char axis = msg.value("axis", "X")[0];
                engine_.jog(axis, msg.value("distance", 0.0), msg.value("speed", 50.0));
            }
            else if (type == "set_temp") {
                if (msg.value("target", "") == "bed")
                    engine_.set_bed_temp(msg.value("temp", 0.0));
                else
                    engine_.set_nozzle_temp(msg.value("nozzle", 0), msg.value("temp", 0.0));
            }
            else if (type == "set_speed") {
                /* TODO: speed override */
            }
            else if (type == "set_flow") {
                /* TODO: flow override */
            }
            else if (type == "set_fan") {
                engine_.set_fan_speed(0, msg.value("percent", 0));
            }
        });

    /* ── Static files (React UI) ────────────────────────────────── */

    CROW_CATCHALL_ROUTE(app)(
        [this](const crow::request& req) {
            /* Serve static files from ui/dist/ */
            std::string url_path = req.url;
            if (url_path == "/") url_path = "/index.html";

            std::string file_path = static_dir_ + url_path;

            /* Security: prevent path traversal */
            auto canonical = fs::weakly_canonical(file_path);
            auto base = fs::weakly_canonical(static_dir_);
            if (canonical.string().find(base.string()) != 0) {
                return crow::response(403);
            }

            if (!fs::exists(canonical) || fs::is_directory(canonical)) {
                /* SPA fallback: serve index.html for client-side routing */
                file_path = static_dir_ + "/index.html";
                if (!fs::exists(file_path)) {
                    return crow::response(404, "UI not built. Run: cd ui && npm run build");
                }
            }

            /* Read file */
            std::ifstream f(file_path, std::ios::binary);
            if (!f) return crow::response(500);
            std::string content((std::istreambuf_iterator<char>(f)),
                                 std::istreambuf_iterator<char>());

            /* Content type */
            auto ext = fs::path(file_path).extension().string();
            std::string content_type = "application/octet-stream";
            if (ext == ".html") content_type = "text/html; charset=utf-8";
            else if (ext == ".js")  content_type = "application/javascript";
            else if (ext == ".css") content_type = "text/css";
            else if (ext == ".svg") content_type = "image/svg+xml";
            else if (ext == ".json") content_type = "application/json";
            else if (ext == ".png") content_type = "image/png";
            else if (ext == ".ico") content_type = "image/x-icon";
            else if (ext == ".woff2") content_type = "font/woff2";
            else if (ext == ".woff") content_type = "font/woff";

            auto resp = crow::response(200, content);
            resp.set_header("Content-Type", content_type);
            return resp;
        });

    /* Run Crow (blocks until stop) */
    app.port(port_)
       .multithreaded()
       .run();
}

void WebServer::broadcast_thread() {
    while (running_) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100)); /* 10Hz */

        std::string payload = build_status_json().dump();

        std::lock_guard lock(clients_mutex_);
        for (auto it = clients_.begin(); it != clients_.end(); ) {
            try {
                (*it)->conn->send_text(payload);
                ++it;
            } catch (...) {
                delete *it;
                it = clients_.erase(it);
            }
        }
    }
}

} // namespace hydra::web
