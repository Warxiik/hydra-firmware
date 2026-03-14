#include "ota.h"
#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>
#include <fstream>
#include <cstdlib>
#include <array>

namespace fs = std::filesystem;
using json = nlohmann::json;

namespace hydra {

OtaManager::OtaManager(const std::string& update_dir)
    : update_dir_(update_dir) {
    fs::create_directories(update_dir_);

    /* Read current version from version file if it exists */
    auto version_file = fs::path("/etc/hydra/version");
    if (fs::exists(version_file)) {
        std::ifstream f(version_file);
        if (f) std::getline(f, current_version_);
    }
}

void OtaManager::set_progress(OtaState state, int percent, const std::string& msg) {
    progress_.state = state;
    progress_.percent = percent;
    progress_.message = msg;
    spdlog::info("OTA: [{}%] {}", percent, msg);
}

bool OtaManager::check_update(const std::string& url_or_path) {
    /* Check local file */
    if (fs::exists(url_or_path)) {
        auto manifest_path = fs::path(url_or_path);
        if (manifest_path.extension() == ".json") {
            std::ifstream f(manifest_path);
            if (!f) return false;
            auto manifest = json::parse(f, nullptr, false);
            if (manifest.is_discarded()) return false;
            available_version_ = manifest.value("version", "");
            return update_available();
        }
    }

    /* URL check: use curl to fetch manifest */
    std::string cmd = "curl -sf " + url_or_path + "/manifest.json 2>/dev/null";
    std::array<char, 4096> buf;
    std::string result;

    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) return false;
    while (fgets(buf.data(), buf.size(), pipe)) {
        result += buf.data();
    }
    int status = pclose(pipe);
    if (status != 0) return false;

    auto manifest = json::parse(result, nullptr, false);
    if (manifest.is_discarded()) return false;

    available_version_ = manifest.value("version", "");
    progress_.version = available_version_;
    return update_available();
}

bool OtaManager::upload_and_install(const std::string& filename, const std::string& data) {
    auto pkg_path = update_dir_ / filename;

    /* Write uploaded data to file */
    std::ofstream f(pkg_path, std::ios::binary);
    if (!f) {
        set_progress(OtaState::Error, 0, "Failed to write update package");
        return false;
    }
    f.write(data.data(), data.size());
    f.close();

    return start_update(pkg_path.string());
}

bool OtaManager::start_update(const std::string& package_path) {
    if (!fs::exists(package_path)) {
        set_progress(OtaState::Error, 0, "Package not found: " + package_path);
        return false;
    }

    set_progress(OtaState::Verifying, 10, "Verifying update package...");

    if (!verify_package(package_path)) {
        set_progress(OtaState::Error, 0, "Package verification failed");
        return false;
    }

    set_progress(OtaState::Installing, 20, "Extracting update...");

    auto extract_dir = update_dir_ / "staging";
    fs::create_directories(extract_dir);

    if (!extract_package(package_path)) {
        set_progress(OtaState::Error, 0, "Extraction failed");
        return false;
    }

    /* Install host binary if present */
    auto host_binary = extract_dir / "hydra-host";
    if (fs::exists(host_binary)) {
        set_progress(OtaState::Installing, 40, "Installing host firmware...");
        if (!install_host_binary(host_binary)) {
            set_progress(OtaState::Error, 0, "Host binary installation failed");
            return false;
        }
    }

    /* Flash MCU firmware if present */
    auto mcu_firmware = extract_dir / "hydra-mcu.uf2";
    if (fs::exists(mcu_firmware)) {
        set_progress(OtaState::Installing, 70, "Flashing MCU firmware...");
        if (!flash_mcu_firmware(mcu_firmware)) {
            set_progress(OtaState::Error, 0, "MCU flash failed");
            return false;
        }
    }

    /* Read version from manifest */
    auto manifest_path = extract_dir / "manifest.json";
    if (fs::exists(manifest_path)) {
        std::ifstream f(manifest_path);
        auto manifest = json::parse(f, nullptr, false);
        if (!manifest.is_discarded()) {
            std::string new_version = manifest.value("version", current_version_);
            /* Write new version file */
            fs::create_directories("/etc/hydra");
            std::ofstream vf("/etc/hydra/version");
            if (vf) vf << new_version;
            current_version_ = new_version;
        }
    }

    /* Clean up staging */
    fs::remove_all(extract_dir);

    set_progress(OtaState::Complete, 100, "Update complete. Restart required.");
    return true;
}

bool OtaManager::verify_package(const fs::path& package_path) {
    /* Verify the package is a valid tar.gz by checking magic bytes */
    std::ifstream f(package_path, std::ios::binary);
    if (!f) return false;

    /* gzip magic: 1f 8b */
    unsigned char magic[2];
    f.read(reinterpret_cast<char*>(magic), 2);
    if (magic[0] == 0x1f && magic[1] == 0x8b) return true;

    /* Also accept plain tar (ustar magic at offset 257) */
    f.seekg(257);
    char ustar[5];
    f.read(ustar, 5);
    if (std::string(ustar, 5) == "ustar") return true;

    /* Accept .uf2 files directly (UF2 magic: 0x0A324655) */
    f.seekg(0);
    uint32_t uf2_magic;
    f.read(reinterpret_cast<char*>(&uf2_magic), 4);
    if (uf2_magic == 0x0A324655) return true;

    return false;
}

bool OtaManager::extract_package(const fs::path& package_path) {
    auto staging = update_dir_ / "staging";
    fs::create_directories(staging);

    /* If it's a .uf2 file, just copy it directly */
    if (package_path.extension() == ".uf2") {
        fs::copy(package_path, staging / "hydra-mcu.uf2",
                 fs::copy_options::overwrite_existing);
        return true;
    }

    /* Extract tar.gz */
    std::string cmd = "tar -xzf " + package_path.string() +
                      " -C " + staging.string() + " 2>&1";
    int ret = std::system(cmd.c_str());
    return ret == 0;
}

bool OtaManager::install_host_binary(const fs::path& binary_path) {
    auto target = fs::path("/usr/local/bin/hydra-host");

    /* Make backup of current binary */
    if (fs::exists(target)) {
        auto backup = target;
        backup += ".bak";
        std::error_code ec;
        fs::copy(target, backup, fs::copy_options::overwrite_existing, ec);
        if (ec) {
            spdlog::warn("OTA: Could not backup current binary: {}", ec.message());
        }
    }

    /* Copy new binary */
    std::error_code ec;
    fs::copy(binary_path, target, fs::copy_options::overwrite_existing, ec);
    if (ec) {
        spdlog::error("OTA: Failed to install host binary: {}", ec.message());
        return false;
    }

    /* Set executable permission */
    fs::permissions(target, fs::perms::owner_exec | fs::perms::group_exec | fs::perms::others_exec,
                    fs::perm_options::add, ec);

    set_progress(OtaState::Installing, 60, "Host binary installed");
    return true;
}

bool OtaManager::flash_mcu_firmware(const fs::path& uf2_path) {
    /*
     * Flash RP2040 via picotool or by copying to BOOTSEL mass storage.
     *
     * Method 1: picotool (preferred — can reboot into BOOTSEL automatically)
     *   picotool load <file.uf2> -f
     *
     * Method 2: Manual BOOTSEL
     *   The MCU must be in BOOTSEL mode (GPIO-triggered or via picotool reboot -u).
     *   Then the uf2 is copied to the mounted mass storage device.
     */

    /* Try picotool first */
    std::string cmd = "picotool load " + uf2_path.string() + " -f 2>&1";
    int ret = std::system(cmd.c_str());
    if (ret == 0) {
        /* Reboot the MCU */
        std::system("picotool reboot 2>&1");
        set_progress(OtaState::Installing, 90, "MCU firmware flashed");
        return true;
    }

    /* Fallback: copy to BOOTSEL mass storage */
    auto bootsel_path = fs::path("/media/RPI-RP2");
    if (fs::exists(bootsel_path) && fs::is_directory(bootsel_path)) {
        std::error_code ec;
        fs::copy(uf2_path, bootsel_path / "hydra-mcu.uf2",
                 fs::copy_options::overwrite_existing, ec);
        if (!ec) {
            set_progress(OtaState::Installing, 90, "MCU firmware copied to BOOTSEL");
            return true;
        }
    }

    spdlog::error("OTA: MCU flash failed — picotool not available and BOOTSEL not mounted");
    return false;
}

} // namespace hydra
