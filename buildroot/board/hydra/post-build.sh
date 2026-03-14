#!/bin/bash
# Hydra — Buildroot post-build script
# Runs after rootfs is assembled, before image generation.

set -euo pipefail

TARGET_DIR="$1"

# Create hydra user and group
if ! grep -q "^hydra:" "${TARGET_DIR}/etc/passwd" 2>/dev/null; then
    echo "hydra:x:1000:1000:Hydra Printer:/home/hydra:/bin/sh" >> "${TARGET_DIR}/etc/passwd"
    echo "hydra:x:1000:" >> "${TARGET_DIR}/etc/group"
    mkdir -p "${TARGET_DIR}/home/hydra/gcodes"
    chown -R 1000:1000 "${TARGET_DIR}/home/hydra" 2>/dev/null || true
fi

# Add hydra to required groups (spi, gpio, video, dialout)
for group in spi gpio video dialout input; do
    if grep -q "^${group}:" "${TARGET_DIR}/etc/group"; then
        sed -i "s/^${group}:.*$/&,hydra/" "${TARGET_DIR}/etc/group"
    else
        echo "${group}:x:$(( RANDOM % 900 + 100 )):hydra" >> "${TARGET_DIR}/etc/group"
    fi
done

# Create required directories
mkdir -p "${TARGET_DIR}/var/log/hydra"
mkdir -p "${TARGET_DIR}/var/lib/hydra/updates"
mkdir -p "${TARGET_DIR}/etc/hydra"

# Enable SPI overlay in config.txt
BOOT_DIR="${BINARIES_DIR}"
if [ -f "${BOOT_DIR}/config.txt" ]; then
    if ! grep -q "dtparam=spi=on" "${BOOT_DIR}/config.txt"; then
        echo "dtparam=spi=on" >> "${BOOT_DIR}/config.txt"
    fi
    # Enable DSI display
    if ! grep -q "dtoverlay=vc4-kms-dsi-7inch" "${BOOT_DIR}/config.txt"; then
        echo "dtoverlay=vc4-kms-dsi-7inch" >> "${BOOT_DIR}/config.txt"
    fi
fi

# Enable systemd services
mkdir -p "${TARGET_DIR}/etc/systemd/system/multi-user.target.wants"
ln -sf /etc/systemd/system/hydra-host.service \
    "${TARGET_DIR}/etc/systemd/system/multi-user.target.wants/hydra-host.service"

mkdir -p "${TARGET_DIR}/etc/systemd/system/graphical.target.wants"
ln -sf /etc/systemd/system/hydra-ui.service \
    "${TARGET_DIR}/etc/systemd/system/graphical.target.wants/hydra-ui.service"

# Set hostname
echo "hydra" > "${TARGET_DIR}/etc/hostname"

echo "Hydra post-build complete"
