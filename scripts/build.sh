#!/usr/bin/env sh
set -eu

TARGET="${1:-lab_exception_frame}"

# Reuses build/ if it already exists, so only changed files get recompiled.
# Run scripts/clean.sh first for a full rebuild.
cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=cmake/arm-none-eabi-toolchain.cmake
cmake --build build --target "$TARGET"

ELF=$(find build -type f -name "$TARGET" | head -n1)
if [ -z "$ELF" ]; then
    echo "scripts/build.sh: no ELF named '$TARGET' under build/ (not a firmware target?)" >&2
    exit 1
fi

echo
echo "== readelf -S: $TARGET section headers =="
arm-none-eabi-readelf -S "$ELF"

echo
echo "== objdump -h: $TARGET section summary =="
arm-none-eabi-objdump -h "$ELF"
