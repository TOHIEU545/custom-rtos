#!/usr/bin/env bash
set -euo pipefail

# Xác định thư mục gốc project dựa trên vị trí thực của script.
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"

# Firmware, target và board do build.sh tạo ra ở lần build gần nhất.
FIRMWARE_DIR="${ROOT_DIR}/build/firmware"
ELF="${FIRMWARE_DIR}/current.elf"
TARGET_FILE="${FIRMWARE_DIR}/current.target"
BOARD_FILE="${FIRMWARE_DIR}/current.board"

cd "${ROOT_DIR}"

# Script không nhận tham số, chỉ nạp firmware hiện tại.
if [[ $# -ne 0 ]]; then
    echo "usage: scripts/flash.sh" >&2
    exit 1
fi

# Chưa build lần nào (hoặc build.sh chưa chạy thành công) -> không có gì để nạp.
if [[ ! -s "${ELF}" ]]; then
    echo "scripts/flash.sh: firmware not found:" >&2
    echo "  ${ELF}" >&2
    echo >&2
    echo "Build one first, for example:" >&2
    echo "  ./scripts/build.sh example_port_validation STM32F411E-DISCO" >&2
    exit 1
fi

# Không đoán board từ tên target hoặc ELF: luôn dùng metadata do build.sh ghi.
if [[ ! -s "${BOARD_FILE}" ]]; then
    echo "scripts/flash.sh: board metadata not found:" >&2
    echo "  ${BOARD_FILE}" >&2
    echo "Run scripts/build.sh again before flashing." >&2
    exit 1
fi

BOARD="$(<"${BOARD_FILE}")"
case "${BOARD}" in
    STM32F103C8T6)
        OPENOCD_CFG="${ROOT_DIR}/tools/openocd/stm32f103c8t6.cfg"
        ;;
    STM32F411E-DISCO)
        OPENOCD_CFG="${ROOT_DIR}/tools/openocd/stm32f411e-disco.cfg"
        ;;
    *)
        echo "scripts/flash.sh: unsupported board: ${BOARD}" >&2
        exit 1
        ;;
esac

# Thiếu file cấu hình OpenOCD thì không thể kết nối board.
if [[ ! -f "${OPENOCD_CFG}" ]]; then
    echo "scripts/flash.sh: OpenOCD config not found:" >&2
    echo "  ${OPENOCD_CFG}" >&2
    exit 1
fi

# Đọc tên target chỉ để hiển thị thông tin, không có thì mặc định "unknown".
if [[ -s "${TARGET_FILE}" ]]; then
    TARGET="$(<"${TARGET_FILE}")"
else
    TARGET="unknown"
fi

# Nếu có lệnh `file`, kiểm tra thêm định dạng đúng là ELF ARM trước khi nạp.
if command -v file >/dev/null 2>&1; then
    FILE_INFO="$(file -b "${ELF}")"

    case "${FILE_INFO}" in
        *ELF*ARM*)
            ;;
        *)
            echo "scripts/flash.sh: current.elf is not an ARM ELF:" >&2
            echo "  ${FILE_INFO}" >&2
            exit 1
            ;;
    esac
fi

echo "== firmware to flash =="
echo "Board       : ${BOARD}"
echo "Target      : ${TARGET}"
echo "ELF         : ${ELF}"
echo "OpenOCD cfg : ${OPENOCD_CFG}"
echo

# Nạp firmware qua OpenOCD: ghi, verify, reset board rồi thoát.
openocd \
    -f "${OPENOCD_CFG}" \
    -c "program ${ELF} verify reset exit"

echo
echo "== flash successful =="
echo "Board  : ${BOARD}"
echo "Target : ${TARGET}"
echo "ELF    : ${ELF}"
