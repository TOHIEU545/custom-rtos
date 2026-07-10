cmake_minimum_required(VERSION 3.22)
project(ntos LANGUAGES C ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(NTOS_APP "lab_exception_frame" CACHE STRING "Lab/example target")

include(cmake/ntos_compiler_options.cmake)
include(cmake/ntos_firmware.cmake)

add_subdirectory(third_party/cmsis)
add_subdirectory(bsp/stm32f103c8)
add_subdirectory(kernel)
add_subdirectory(port/cortex_m3)
add_subdirectory(labs)
add_subdirectory(examples)
add_subdirectory(tests)
