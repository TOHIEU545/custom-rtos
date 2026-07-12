cmake_minimum_required(VERSION 3.28)

# Chon kien truc
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)

# Chon compiler
set(CMAKE_C_COMPILER arm-none-eabi-gcc)
set(CMAKE_OBJCOPY arm-none-eabi-objcopy)
set(CMAKE_SIZE arm-none-eabi-size)
set(CMAKE_DUMP arm-none-eabi-objdump)

# Khi ktra compiler, chi compile thanh static lib, khong thu link thanh firmware
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

# Nhung gi lien quan den cross-compile phai dat truoc project
project(lesson_04 LANGUAGES C)

# Chon cac chuan C
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Tao variable de tai su dung
set(PROJECT_ROOT ${CMAKE_CURRENT_LIST_DIR}/../..)
set(TARGET_NAME lesson_04)
set(MCU_FLAGS
    -mcpu=cortex-m3
    -mthumb
)

# ============================================================================
# SPL static library
# ============================================================================

add_library(stm32_spl STATIC
    "${PROJECT_ROOT}/driver/STM32F10x_StdPeriph_Driver/src/stm32f10x_gpio.c"
    "${PROJECT_ROOT}/driver/STM32F10x_StdPeriph_Driver/src/stm32f10x_rcc.c"
)

target_include_directories(stm32_spl PUBLIC
    "${PROJECT_ROOT}/driver/CMSIS/Include"
    "${PROJECT_ROOT}/driver/CMSIS/Device/ST/STM32F1xx/Include"
    "${PROJECT_ROOT}/driver/STM32F10x_StdPeriph_Driver/inc"
    "${PROJECT_ROOT}/bsp/stm32f103c8/include"
)

target_compile_definitions(stm32_spl PUBLIC
    STM32F10X_MD
    USE_STDPERIPH_DRIVER
)

target_compile_options(stm32_spl PRIVATE
    ${MCU_FLAGS}
    -ffunction-sections
    -fdata-sections
    -Wall
    -Wextra
)

# ============================================================================
# LED static library
# ============================================================================

add_library(led_driver STATIC
    "${CMAKE_CURRENT_LIST_DIR}/src/led.c"
)

target_include_directories(led_driver PUBLIC
    "${CMAKE_CURRENT_LIST_DIR}/include"
    "${PROJECT_ROOT}/bsp/stm32f103c8/include"
)

target_compile_options(led_driver PRIVATE
    ${MCU_FLAGS}
    -ffunction-sections
    -fdata-sections
    -Wall
    -Wextra
)

target_link_libraries(led_driver PRIVATE
    stm32_spl
)

# ============================================================================
# Firmware executable
# ============================================================================

add_executable(${TARGET_NAME}
    main.c
    "${PROJECT_ROOT}/bsp/stm32f103c8/startup/startup_stm32f103.c"
    "${PROJECT_ROOT}/bsp/stm32f103c8/system/system_stm32f10x.c"
)

# Tao thanh .elf
set_target_properties(${TARGET_NAME} PROPERTIES
    SUFFIX ".elf"
)

target_link_libraries(${TARGET_NAME} PRIVATE
    led_driver
)

target_compile_options(${TARGET_NAME} PRIVATE
    ${MCU_FLAGS}
    -ffunction-sections
    -fdata-sections
    -Wall
    -Wextra
)

target_include_directories(${TARGET_NAME} PRIVATE
    "${PROJECT_ROOT}/driver/CMSIS/Include"
    "${PROJECT_ROOT}/driver/CMSIS/Device/ST/STM32F1xx/Include"
    "${PROJECT_ROOT}/driver/STM32F10x_StdPeriph_Driver/inc"
    "${PROJECT_ROOT}/bsp/stm32f103c8/include"
)

target_compile_definitions(${TARGET_NAME} PRIVATE
    STM32F10X_MD
    USE_STDPERIPH_DRIVER
)

# Them linker option
target_link_options(${TARGET_NAME} PRIVATE
    ${MCU_FLAGS}
    -nostartfiles
    -Wl,--gc-sections
    -Wl,-Map=${TARGET_NAME}.map
    -T${PROJECT_ROOT}/bsp/stm32f103c8/linker/stm32f103c8.ld
)

# Yeu cau chay cac cmd nay sau khi link thanh cong
add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O ihex $<TARGET_FILE:${TARGET_NAME}> ${TARGET_NAME}.hex
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${TARGET_NAME}> ${TARGET_NAME}.bin
    COMMAND ${CMAKE_SIZE} $<TARGET_FILE:${TARGET_NAME}>
    COMMAND ${CMAKE_OBJDUMP} -h $<TARGET_FILE:${TARGET_NAME}>
)
