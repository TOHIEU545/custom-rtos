cmake_minimum_required(VERSION 3.22)

project(my_rtos LANGUAGES C ASM)

# Project defaults.
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Build groups.
option(RTOS_BUILD_EXAMPLES "Build MyRTOS examples" ON)
option(RTOS_BUILD_TESTS "Build MyRTOS tests" OFF)

# Common Cortex-M3 build flags for this standalone firmware repository.
add_compile_options(
    -mcpu=cortex-m3
    -mthumb
    -g
    -O0
    -ffunction-sections
    -fdata-sections
    -fno-common
    -Wall
    -Wextra
    -Wshadow
    -Wundef)

add_link_options(
    -mcpu=cortex-m3
    -mthumb
    -Wl,--gc-sections)

# Platform module: BSP, startup, linker script.
add_subdirectory(boards/STM32F103)

# OS modules are needed by examples and target tests.
if(RTOS_BUILD_EXAMPLES OR RTOS_BUILD_TESTS)
    add_subdirectory(port/ARM/cortex-m3)
    add_subdirectory(kernel)
endif()

# Firmware groups.
if(RTOS_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

if(RTOS_BUILD_TESTS)
    add_subdirectory(tests)
endif()

