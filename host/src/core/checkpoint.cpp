#include "checkpoint.h"
#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>
#include <fstream>
#include <filesystem>

namespace fs = std::filesystem;
using json = nlohmann::json;

namespace hydra {

CheckpointManager::CheckpointManager(const std::string& checkpoint_dir)
    : dir_(checkpoint_dir)
{
    std::error_code ec;
    fs::create_directories(dir_, ec);
}

std::string CheckpointManager::checkpoint_path() const {
    return (fs::path(dir_) / "print_checkpoint.json").string();
}

bool CheckpointManager::exists() const {
    return fs::exists(checkpoint_path());
}

bool CheckpointManager::save(const PrintCheckpoint& cp) {
    json j;
    j["gcode_path"] = cp.gcode_path;
    j["nozzle0_line"] = cp.nozzle0_line;
    j["nozzle1_line"] = cp.nozzle1_line;
    j["pos_x"] = cp.pos_x;
    j["pos_y"] = cp.pos_y;
    j["pos_z"] = cp.pos_z;
    j["e0"] = cp.e0;
    j["e1"] = cp.e1;
    j["nozzle0_target"] = cp.nozzle0_target;
    j["nozzle1_target"] = cp.nozzle1_target;
    j["bed_target"] = cp.bed_target;
    j["fan0_speed"] = cp.fan0_speed;
    j["fan1_speed"] = cp.fan1_speed;
    j["layer"] = cp.layer;
    j["progress_pct"] = cp.progress_pct;
    j["elapsed_s"] = cp.elapsed_s;

    /*
     * Write to temp file first, then rename for atomicity.
     * This prevents corruption if power is lost mid-write.
     */
    std::string tmp_path = checkpoint_path() + ".tmp";

    std::ofstream f(tmp_path);
    if (!f) {
        spdlog::error("Checkpoint: failed to open temp file for writing");
        return false;
    }

    f << j.dump(2);
    f.flush();
    f.close();

    if (!f.good()) {
        spdlog::error("Checkpoint: write error");
        return false;
    }

    /* Atomic rename */
    std::error_code ec;
    fs::rename(tmp_path, checkpoint_path(), ec);
    if (ec) {
        spdlog::error("Checkpoint: rename failed: {}", ec.message());
        return false;
    }

    spdlog::debug("Checkpoint saved: layer={}, z={:.2f}mm", cp.layer, cp.pos_z);
    return true;
}

bool CheckpointManager::load(PrintCheckpoint& cp_out) {
    if (!exists()) return false;

    std::ifstream f(checkpoint_path());
    if (!f) {
        spdlog::warn("Checkpoint: file exists but cannot be opened");
        return false;
    }

    json j;
    try {
        j = json::parse(f);
    } catch (const json::parse_error& e) {
        spdlog::error("Checkpoint: parse error: {}", e.what());
        return false;
    }

    cp_out.gcode_path = j.value("gcode_path", "");
    cp_out.nozzle0_line = j.value("nozzle0_line", 0u);
    cp_out.nozzle1_line = j.value("nozzle1_line", 0u);
    cp_out.pos_x = j.value("pos_x", 0.0);
    cp_out.pos_y = j.value("pos_y", 0.0);
    cp_out.pos_z = j.value("pos_z", 0.0);
    cp_out.e0 = j.value("e0", 0.0);
    cp_out.e1 = j.value("e1", 0.0);
    cp_out.nozzle0_target = j.value("nozzle0_target", 0.0);
    cp_out.nozzle1_target = j.value("nozzle1_target", 0.0);
    cp_out.bed_target = j.value("bed_target", 0.0);
    cp_out.fan0_speed = j.value("fan0_speed", 0);
    cp_out.fan1_speed = j.value("fan1_speed", 0);
    cp_out.layer = j.value("layer", 0u);
    cp_out.progress_pct = j.value("progress_pct", 0.0);
    cp_out.elapsed_s = j.value("elapsed_s", 0.0);

    spdlog::info("Checkpoint loaded: {} at layer {}, z={:.2f}mm",
                 cp_out.gcode_path, cp_out.layer, cp_out.pos_z);
    return true;
}

void CheckpointManager::clear() {
    std::error_code ec;
    fs::remove(checkpoint_path(), ec);
    fs::remove(checkpoint_path() + ".tmp", ec);
    spdlog::info("Checkpoint cleared");
}

} // namespace hydra
