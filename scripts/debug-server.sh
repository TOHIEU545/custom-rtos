#!/usr/bin/env bash
set -euo pipefail

# Xác định thư mục gốc project dựa trên vị trí thực của script.
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"

OPENOCD_CFG="${ROOT_DIR}/tools/openocd/stm32f103.cfg"

cd "${ROOT_DIR}"

# Cờ nội bộ: đánh dấu tiến trình này đang chạy trong terminal mới do chính
# script tự mở ra, để không mở lặp vô hạn.
SPAWNED_FLAG="--in-new-terminal"
ALREADY_SPAWNED=0
if [[ "${1:-}" == "${SPAWNED_FLAG}" ]]; then
    ALREADY_SPAWNED=1
    shift
fi

if [[ $# -ne 0 ]]; then
    echo "usage: scripts/debug-server.sh" >&2
    exit 1
fi

# Thiếu file cấu hình OpenOCD thì không thể kết nối board.
if [[ ! -f "${OPENOCD_CFG}" ]]; then
    echo "scripts/debug-server.sh: OpenOCD config not found:" >&2
    echo "  ${OPENOCD_CFG}" >&2
    exit 1
fi

# Terminal gọi lệnh này (vd: terminal tích hợp VSCode) cần rảnh cho
# scripts/debug.sh, nên tự mở một terminal mới để chạy OpenOCD ở đó.
if [[ "${ALREADY_SPAWNED}" -eq 0 ]] && command -v gnome-terminal >/dev/null 2>&1; then
    exec gnome-terminal --title="MyRTOS OpenOCD" -- "${SCRIPT_DIR}/debug-server.sh" "${SPAWNED_FLAG}"
fi

if [[ "${ALREADY_SPAWNED}" -eq 0 ]]; then
    echo "scripts/debug-server.sh: không tìm thấy gnome-terminal, chạy OpenOCD ngay tại đây." >&2
fi

echo "== OpenOCD debug server =="
echo "Config   : ${OPENOCD_CFG}"
echo "GDB port : 3333"
echo
echo "Giữ terminal này chạy. Chạy scripts/debug.sh ở terminal hiện tại để attach GDB."
echo

# exec để OpenOCD nhận trực tiếp tín hiệu (Ctrl+C) thay vì một shell con,
# và giữ server chạy ở terminal này cho tới khi bị dừng.
exec openocd \
    -f "${OPENOCD_CFG}" \
    -c "gdb_port 3333"
