#!/usr/bin/env sh
set -eu
ELF="${1:?usage: scripts/debug.sh firmware.elf}"
arm-none-eabi-gdb "$ELF" -x tools/gdb/ntos.gdb
