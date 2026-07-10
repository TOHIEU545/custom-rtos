#!/usr/bin/env sh
set -eu
APP="${1:-lab_exception_frame}"
cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=cmake/arm-none-eabi-toolchain.cmake -DNTOS_APP="$APP"
cmake --build build
