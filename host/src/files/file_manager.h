#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <filesystem>

namespace hydra::files {

struct GcodeFileInfo {
    std::string name;           /* Filename (without path) */
    std::string path;           /* Full path */
    uint64_t size_bytes = 0;    /* File size */
    std::string modified;       /* ISO 8601 timestamp */
    bool is_dual = false;       /* True if _nozzle0/_nozzle1 pair exists */
};

/**
 * Manages G-code files in the printer's gcode directory.
 * Supports listing, uploading, deleting, and detecting dual-nozzle file pairs.
 */
class FileManager {
public:
    explicit FileManager(const std::string& gcode_dir);

    /** List all G-code files in the directory. */
    std::vector<GcodeFileInfo> list_files() const;

    /** Check if a file exists. */
    bool file_exists(const std::string& filename) const;

    /** Get full path for a filename. */
    std::string full_path(const std::string& filename) const;

    /** Save uploaded file data. Returns true on success. */
    bool save_file(const std::string& filename, const std::string& data);

    /** Delete a file. Returns true on success. */
    bool delete_file(const std::string& filename);

    /** Get total and free disk space in bytes. */
    struct DiskSpace {
        uint64_t total = 0;
        uint64_t free = 0;
    };
    DiskSpace disk_space() const;

private:
    /** Validate filename (no path traversal, valid extension). */
    bool is_valid_filename(const std::string& filename) const;

    /** Format file modification time as ISO 8601. */
    static std::string format_time(std::filesystem::file_time_type ftime);

    /** Check if a dual-nozzle pair exists for a base name. */
    bool has_dual_pair(const std::string& base_name) const;

    std::filesystem::path gcode_dir_;
};

} // namespace hydra::files
