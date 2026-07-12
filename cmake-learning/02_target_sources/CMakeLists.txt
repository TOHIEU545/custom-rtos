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
project(lesson_02_target_sources LANGUAGES C)

# Chon cac chuan C
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Tao variable de tai su dung
set(PROJECT_ROOT ${CMAKE_CURRENT_LIST_DIR}/../..)
set(TARGET_NAME lesson_02_target_sources)

# Tao ra tep chuong trinh thuc thi
add_executable(${TARGET_NAME})

# Tao thanh .elf
set_target_properties(${TARGET_NAME} PROPERTIES
    SUFFIX ".elf"
)

target_sources(${TARGET_NAME} PRIVATE
    main.c

    ${PROJECT_ROOT}/bsp/stm32f103c8/startup/startup_stm32f103.c
    ${PROJECT_ROOT}/bsp/stm32f103c8/system/system_stm32f10x.c
    ${PROJECT_ROOT}/driver/STM32F10x_StdPeriph_Driver/src/stm32f10x_gpio.c
    ${PROJECT_ROOT}/driver/STM32F10x_StdPeriph_Driver/src/stm32f10x_rcc.c

    led.c
)

# Them cac include directory
target_include_directories(${TARGET_NAME} PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
    ${PROJECT_ROOT}/driver/CMSIS/Include
    ${PROJECT_ROOT}/driver/CMSIS/Device/ST/STM32F1xx/Include
    ${PROJECT_ROOT}/driver/STM32F10x_StdPeriph_Driver/inc
    ${PROJECT_ROOT}/bsp/stm32f103c8/include
    led.h
)

# Them cac compile definition
target_compile_definitions(${TARGET_NAME} PRIVATE
    STM32F10X_MD
    USE_STDPERIPH_DRIVER
)

# Them cac compile option
target_compile_options(${TARGET_NAME} PRIVATE
    -mcpu=cortex-m3
    -mthumb
    -ffunction-sections
    -fdata-sections
    -Wall
    -Wextra
)

# Them linker option
target_link_options(${TARGET_NAME} PRIVATE
    -mcpu=cortex-m3
    -mthumb
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