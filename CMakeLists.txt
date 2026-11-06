cmake_minimum_required(VERSION 3.22)

project(my_rtos LANGUAGES C ASM)

# Project defaults.
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Build groups.
option(RTOS_BUILD_EXAMPLES "Build MyRTOS examples" ON)

# Common build flags shared by every supported architecture.
add_compile_options(
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
    -Wl,--gc-sections)

# Select one concrete board. Each board chooses its matching CPU port and owns
# its device define, startup code and linker script.
set(RTOS_BOARD "STM32F103C8T6" CACHE STRING "Target board")
set_property(CACHE RTOS_BOARD PROPERTY STRINGS STM32F103C8T6)

if(RTOS_BOARD STREQUAL "STM32F103C8T6")
    add_subdirectory(port/ARM/cortex-m3)
    add_subdirectory(boards/STM32F103C8T6)
else()
    message(FATAL_ERROR "Unsupported RTOS_BOARD: ${RTOS_BOARD}")
endif()

# Portable kernel: it receives architecture requirements through rtos_port.
add_subdirectory(kernel)

# Firmware groups.
if(RTOS_BUILD_EXAMPLES)
    add_subdirectory(examples/${RTOS_BOARD})
endif()

