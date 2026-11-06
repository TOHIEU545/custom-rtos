#!/usr/bin/env sh
set -eu

# Tham số thứ hai là board; bỏ qua thì dùng board hiện tại của project.
if [ "$#" -lt 1 ] || [ "$#" -gt 2 ]; then
    echo "usage: scripts/build.sh <firmware-target> [board]" >&2
    echo "  e.g. scripts/build.sh example_scheduler_demo STM32F103C8T6" >&2
    exit 1
fi

TARGET="$1"
BOARD="${2:-STM32F103C8T6}"

# Xác định thư mục gốc project dựa trên vị trí thực của script,
# để chạy đúng dù người dùng gọi script từ đâu.
SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
ROOT_DIR=$(CDPATH= cd -- "${SCRIPT_DIR}/.." && pwd)

BUILD_ROOT="${ROOT_DIR}/build"
BUILD_DIR="${BUILD_ROOT}/${BOARD}"
FIRMWARE_DIR="${BUILD_ROOT}/firmware"
CURRENT_ELF="${FIRMWARE_DIR}/current.elf"
CURRENT_TARGET="${FIRMWARE_DIR}/current.target"
CURRENT_BOARD="${FIRMWARE_DIR}/current.board"

cd "${ROOT_DIR}"

# Tái sử dụng cây build CMake sẵn có, chỉ những file thay đổi mới được biên dịch lại.
cmake \
    -S "${ROOT_DIR}" \
    -B "${BUILD_DIR}" \
    -DCMAKE_TOOLCHAIN_FILE="${ROOT_DIR}/cmake/arm-none-eabi-toolchain.cmake" \
    -DRTOS_BOARD="${BOARD}" \
    -DRTOS_BUILD_EXAMPLES=ON

# Build đúng target được yêu cầu.
cmake --build "${BUILD_DIR}" --target "${TARGET}"

# Chỉ tìm trong output thật của CMake.
# Bỏ qua firmware/ vì đó là bản copy do chính script này tạo ra.
ELF_CANDIDATES=$(
    find "${BUILD_DIR}" \
        -type f \
        ! -path "${FIRMWARE_DIR}/*" \
        \( -name "${TARGET}" -o -name "${TARGET}.elf" \)
)

CANDIDATE_COUNT=$(
    printf '%s\n' "${ELF_CANDIDATES}" |
        sed '/^$/d' |
        wc -l |
        tr -d ' '
)

# Không tìm thấy ELF nào khớp target -> báo lỗi.
if [ "${CANDIDATE_COUNT}" -eq 0 ]; then
    echo "scripts/build.sh: no ELF found for target '${TARGET}'." >&2
    exit 1
fi

# Tìm thấy nhiều hơn 1 ELF -> không xác định được cái nào đúng, dừng lại.
if [ "${CANDIDATE_COUNT}" -gt 1 ]; then
    echo "scripts/build.sh: multiple ELF candidates found:" >&2
    printf '%s\n' "${ELF_CANDIDATES}" | sed 's/^/  /' >&2
    exit 1
fi

ELF="${ELF_CANDIDATES}"

# Kiểm tra ELF thực sự tồn tại và không rỗng.
if [ ! -s "${ELF}" ]; then
    echo "scripts/build.sh: ELF is missing or empty: ${ELF}" >&2
    exit 1
fi

# Nếu có lệnh `file`, kiểm tra thêm định dạng đúng là ELF ARM.
if command -v file >/dev/null 2>&1; then
    FILE_INFO=$(file -b "${ELF}")

    case "${FILE_INFO}" in
        *ELF*ARM*)
            ;;
        *)
            echo "scripts/build.sh: output is not an ARM ELF:" >&2
            echo "  ${FILE_INFO}" >&2
            exit 1
            ;;
    esac
fi

# In thông tin section và kích thước firmware để kiểm tra nhanh.
echo
echo "== objdump -h: ${TARGET} section summary =="
arm-none-eabi-objdump -h "${ELF}"

echo
arm-none-eabi-size "${ELF}"

# Tạo lại firmware/ sau khi build thành công.
# Đảm bảo thư mục này chỉ chứa đúng firmware hiện tại, không lẫn bản cũ.
rm -rf "${FIRMWARE_DIR}"
mkdir -p "${FIRMWARE_DIR}"

cp "${ELF}" "${CURRENT_ELF}"
printf '%s\n' "${TARGET}" > "${CURRENT_TARGET}"
printf '%s\n' "${BOARD}" > "${CURRENT_BOARD}"

if [ ! -s "${CURRENT_ELF}" ]; then
    echo "scripts/build.sh: failed to create ${CURRENT_ELF}" >&2
    exit 1
fi

echo
echo "== current firmware =="
echo "Board      : ${BOARD}"
echo "Target     : ${TARGET}"
echo "Source ELF : ${ELF}"
echo "Current ELF: ${CURRENT_ELF}"
