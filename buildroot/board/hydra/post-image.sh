#!/bin/bash
# Hydra — Buildroot post-image script
# Generates the final SD card image from the built rootfs.

set -euo pipefail

BOARD_DIR="$(dirname "$0")"
GENIMAGE_CFG="${BOARD_DIR}/genimage.cfg"
GENIMAGE_TMP="${BUILD_DIR}/genimage.tmp"

rm -rf "${GENIMAGE_TMP}"

genimage \
    --rootpath "${TARGET_DIR}" \
    --tmppath "${GENIMAGE_TMP}" \
    --inputpath "${BINARIES_DIR}" \
    --outputpath "${BINARIES_DIR}" \
    --config "${GENIMAGE_CFG}"

echo "SD card image generated: ${BINARIES_DIR}/hydra-sdcard.img"
echo "Flash with: dd if=hydra-sdcard.img of=/dev/sdX bs=4M status=progress"
