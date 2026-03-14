#pragma once

#include <string>
#include <functional>
#include <filesystem>

namespace hydra {

enum class OtaState {
    Idle,
    Downloading,
    Verifying,
    Installing,
    Complete,
    Error,
};

struct OtaProgress {
    OtaState state = OtaState::Idle;
    int percent = 0;
    std::string message;
    std::string version;
};

/**
 * OTA firmware update manager.
 *
 * Handles updating both the host software (via dpkg/systemd restart)
 * and the MCU firmware (.uf2 via BOOTSEL mode).
 *
 * Update packages are .tar.gz containing:
 *   - manifest.json  (version, checksums, component list)
 *   - hydra-host      (ARM64 binary)
 *   - hydra-mcu.uf2   (RP2040 firmware)
 */
class OtaManager {
public:
    explicit OtaManager(const std::string& update_dir = "/var/lib/hydra/updates");

    /* Check for updates from a URL or local file */
    bool check_update(const std::string& url_or_path);

    /* Start update from uploaded package */
    bool start_update(const std::string& package_path);

    /* Start update from uploaded raw data */
    bool upload_and_install(const std::string& filename, const std::string& data);

    /* Current state */
    OtaProgress progress() const { return progress_; }
    std::string current_version() const { return current_version_; }
    std::string available_version() const { return available_version_; }
    bool update_available() const { return !available_version_.empty() && available_version_ != current_version_; }

private:
    bool verify_package(const std::filesystem::path& package_path);
    bool extract_package(const std::filesystem::path& package_path);
    bool install_host_binary(const std::filesystem::path& binary_path);
    bool flash_mcu_firmware(const std::filesystem::path& uf2_path);
    void set_progress(OtaState state, int percent, const std::string& msg);

    std::filesystem::path update_dir_;
    std::string current_version_ = "0.1.0";
    std::string available_version_;
    OtaProgress progress_;
};

} // namespace hydra
