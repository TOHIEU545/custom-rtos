#!/usr/bin/env bash
set -euo pipefail

# Xác định thư mục gốc project dựa trên vị trí thực của script.
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"

# Firmware và target name do build.sh tạo ra ở lần build gần nhất.
FIRMWARE_DIR="${ROOT_DIR}/build/firmware"
ELF="${FIRMWARE_DIR}/current.elf"
TARGET_FILE="${FIRMWARE_DIR}/current.target"

GDB_CMDS="${ROOT_DIR}/tools/gdb/ntos.gdb"

cd "${ROOT_DIR}"

# Script không nhận tham số, chỉ attach vào firmware hiện tại.
if [[ $# -ne 0 ]]; then
    echo "usage: scripts/debug.sh" >&2
    exit 1
fi

# Chưa build lần nào (hoặc build.sh chưa chạy thành công) -> không có gì để debug.
if [[ ! -s "${ELF}" ]]; then
    echo "scripts/debug.sh: firmware not found:" >&2
    echo "  ${ELF}" >&2
    echo >&2
    echo "Build one first, for example:" >&2
    echo "  ./scripts/build.sh example_scheduler_demo" >&2
    exit 1
fi

# Thiếu command file thì gdb không biết connect vào đâu.
if [[ ! -f "${GDB_CMDS}" ]]; then
    echo "scripts/debug.sh: GDB command file not found:" >&2
    echo "  ${GDB_CMDS}" >&2
    exit 1
fi

# Đọc tên target chỉ để hiển thị thông tin, không có thì mặc định "unknown".
if [[ -s "${TARGET_FILE}" ]]; then
    TARGET="$(<"${TARGET_FILE}")"
else
    TARGET="unknown"
fi

echo "== gdb-multiarch debug session =="
echo "Target   : ${TARGET}"
echo "ELF      : ${ELF}"
echo "GDB cmds : ${GDB_CMDS}"
echo
echo "Yêu cầu scripts/debug-server.sh đang chạy ở một terminal khác."
echo

# tools/gdb/ntos.gdb đã có sẵn: connect :3333, reset halt, chạy tới main().
# Không lặp lại các lệnh này ở đây, không tự load lại firmware (đó là việc
# của scripts/flash.sh).
exec gdb-multiarch "${ELF}" -x "${GDB_CMDS}"
