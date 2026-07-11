#!/usr/bin/env bash
set -euo pipefail

# Duong dan toi thu muc chua CmakeList
SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
# Duong dan toi thu muc chua build
BUILD_DIR="${SCRIPT_DIR}/build"

PROJECT_ROOT="${SCRIPT_DIR}/../.."
TARGET_NAME="lesson_01_add_executable"
FIRMWARE_ELF="${BUILD_DIR}/${TARGET_NAME}.elf"
OPENOCD_CFG="${PROJECT_ROOT}/tools/openocd/stm32f103.cfg"
COMMAND="${1:-build}"

if [[ "${COMMAND}" == "clean" ]]; then
    rm -rf "${BUILD_DIR}"
fi

cmake -S "${SCRIPT_DIR}" -B "${BUILD_DIR}" 

cmake --build "${BUILD_DIR}" --parallel

if [[ "${COMMAND}" == "flash" ]]; then
    if [[ ! -f "${FIRMWARE_ELF}" ]]; then
        echo "Error: firmware not found: ${FIRMWARE_ELF}" >&2
        exit 1
    fi

    openocd -f "${OPENOCD_CFG}" \
        -c "program ${FIRMWARE_ELF} verify reset exit"
fi
