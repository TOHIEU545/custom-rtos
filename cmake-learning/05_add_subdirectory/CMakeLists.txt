cmake_minimum_required(VERSION 3.28)

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)
set(CMAKE_C_COMPILER arm-none-eabi-gcc)
set(CMAKE_OBJCOPY arm-none-eabi-objcopy)
set(CMAKE_SIZE arm-none-eabi-size)
set(CMAKE_OBJDUMP arm-none-eabi-objdump)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

project(lesson_05 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(PROJECT_ROOT ${CMAKE_CURRENT_LIST_DIR}/../..)
set(MCU_FLAGS
    -mcpu=cortex-m3
    -mthumb
)
set(COMMON_C_FLAGS
    ${MCU_FLAGS}
    -ffunction-sections
    -fdata-sections
    -Wall
    -Wextra
)

add_subdirectory(stm32_spl)
add_subdirectory(led_driver)
add_subdirectory(app)
