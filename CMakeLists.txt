cmake_minimum_required(VERSION 3.22)

project(ntos LANGUAGES C ASM)

# Project defaults.
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Build groups.
option(NTOS_BUILD_LABS "Build Cortex-M3 learning labs" ON)
option(NTOS_BUILD_EXAMPLES "Build ntos examples" OFF)
option(NTOS_BUILD_TESTS "Build tests" OFF)

# Common CMake helpers.
include(cmake/ntos_compiler_options.cmake)
include(cmake/ntos_firmware.cmake)

# Vendor module: CMSIS + STM32F10x device headers.
add_subdirectory(cmsis)

# Platform module: BSP, startup, linker script.
add_subdirectory(bsp/stm32f10x)

# OS modules are only needed by examples for now.
if(NTOS_BUILD_EXAMPLES)
    add_subdirectory(kernel)
    add_subdirectory(port/cortex_m3)
endif()

# Firmware groups.
if(NTOS_BUILD_LABS)
    add_subdirectory(labs)
endif()

if(NTOS_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

if(NTOS_BUILD_TESTS)
    add_subdirectory(tests)
endif()
