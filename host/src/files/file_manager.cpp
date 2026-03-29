#include "file_manager.h"
#include <spdlog/spdlog.h>
#include <fstream>
#include <algorithm>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

namespace fs = std::filesystem;

namespace hydra::files {

FileManager::FileManager(const std::string& gcode_dir)
    : gcode_dir_(gcode_dir)
{
    /* Create directory if it doesn't exist */
    std::error_code ec;
    fs::create_directories(gcode_dir_, ec);
    if (ec) {
        spdlog::warn("Could not create gcode directory {}: {}", gcode_dir, ec.message());
    }
}

bool FileManager::is_valid_filename(const std::string& filename) const {
    if (filename.empty()) return false;

    /* Reject path traversal */
    if (filename.find("..") != std::string::npos) return false;
    if (filename.find('/') != std::string::npos) return false;
    if (filename.find('\\') != std::string::npos) return false;

    /* Must have a valid extension */
    auto ext = fs::path(filename).extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    return ext == ".gcode" || ext == ".g" || ext == ".gco";
}

std::string FileManager::format_time(fs::file_time_type ftime) {
    auto sctp = std::chrono::time_point_cast<std::chrono::seconds>(
        std::chrono::file_clock::to_sys(ftime));
    auto time_t_val = std::chrono::system_clock::to_time_t(sctp);

    std::tm tm_buf;
#ifdef _WIN32
    gmtime_s(&tm_buf, &time_t_val);
#else
    gmtime_r(&time_t_val, &tm_buf);
#endif

    std::ostringstream oss;
    oss << std::put_time(&tm_buf, "%Y-%m-%dT%H:%M:%SZ");
    return oss.str();
}

std::vector<GcodeFileInfo> FileManager::list_files() const {
    std::vector<GcodeFileInfo> files;
    std::error_code ec;

    if (!fs::exists(gcode_dir_, ec) || !fs::is_directory(gcode_dir_, ec)) {
        return files;
    }

    for (const auto& entry : fs::directory_iterator(gcode_dir_, ec)) {
        if (!entry.is_regular_file()) continue;

        auto ext = entry.path().extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        if (ext != ".gcode" && ext != ".g" && ext != ".gco") continue;

        GcodeFileInfo info;
        info.name = entry.path().filename().string();
        info.path = entry.path().string();
        info.size_bytes = entry.file_size(ec);
        info.modified = format_time(entry.last_write_time(ec));

        files.push_back(std::move(info));
    }

    /* Sort by modification time (newest first) */
    std::sort(files.begin(), files.end(), [](const GcodeFileInfo& a, const GcodeFileInfo& b) {
        return a.modified > b.modified;
    });

    return files;
}

bool FileManager::file_exists(const std::string& filename) const {
    if (!is_valid_filename(filename)) return false;
    return fs::exists(gcode_dir_ / filename);
}

std::string FileManager::full_path(const std::string& filename) const {
    return (gcode_dir_ / filename).string();
}

bool FileManager::save_file(const std::string& filename, const std::string& data) {
    if (!is_valid_filename(filename)) {
        spdlog::warn("Invalid filename for upload: {}", filename);
        return false;
    }

    auto path = gcode_dir_ / filename;
    spdlog::info("Saving uploaded file: {} ({} bytes)", filename, data.size());

    std::ofstream f(path, std::ios::binary);
    if (!f) {
        spdlog::error("Failed to open file for writing: {}", path.string());
        return false;
    }

    f.write(data.data(), static_cast<std::streamsize>(data.size()));
    if (!f.good()) {
        spdlog::error("Write error for file: {}", path.string());
        return false;
    }

    return true;
}

bool FileManager::delete_file(const std::string& filename) {
    if (!is_valid_filename(filename)) {
        spdlog::warn("Invalid filename for delete: {}", filename);
        return false;
    }

    auto path = gcode_dir_ / filename;
    std::error_code ec;

    if (!fs::exists(path, ec)) {
        spdlog::warn("File not found for delete: {}", filename);
        return false;
    }

    if (!fs::remove(path, ec)) {
        spdlog::error("Failed to delete file: {} ({})", filename, ec.message());
        return false;
    }

    spdlog::info("Deleted file: {}", filename);
    return true;
}

FileManager::DiskSpace FileManager::disk_space() const {
    std::error_code ec;
    auto space = fs::space(gcode_dir_, ec);
    if (ec) return {};
    return {space.capacity, space.available};
}

} // namespace hydra::files
