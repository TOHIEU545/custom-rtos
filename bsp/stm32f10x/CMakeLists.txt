# Board support code for the STM32F103 line (STM32F10x family).
add_library(ntos_bsp STATIC
    src/gpio.c src/uart.c src/exti.c
    system/system_stm32f10x.c startup/startup_stm32f103.c)

target_include_directories(ntos_bsp
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/internal)
target_link_libraries(ntos_bsp PUBLIC ntos_compiler_options cmsis)

# Complete STM32F10x firmware platform: BSP + vendor headers + linker script.
add_library(ntos_platform_stm32f10x INTERFACE)
target_link_libraries(ntos_platform_stm32f10x INTERFACE
    ntos_compiler_options
    ntos_bsp
    cmsis)
target_link_options(ntos_platform_stm32f10x INTERFACE
    -T${CMAKE_CURRENT_SOURCE_DIR}/linker/stm32f10x.ld)
