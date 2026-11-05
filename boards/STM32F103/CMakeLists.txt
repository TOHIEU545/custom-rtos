# STM32F103 board support, startup and linker configuration.
add_library(rtos_bsp STATIC
    bsp.c
    startup.c
    vendor/ST/STM32F10x/Source/system_stm32f10x.c)

target_include_directories(rtos_bsp
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/cmsis/Core/Include
        ${CMAKE_CURRENT_SOURCE_DIR}/vendor/ST/STM32F10x/Include)

target_compile_definitions(rtos_bsp PRIVATE STM32F10X_MD)

add_library(rtos_platform_stm32f103 INTERFACE)
target_link_libraries(rtos_platform_stm32f103 INTERFACE
    rtos_bsp)
target_link_options(rtos_platform_stm32f103 INTERFACE
    -T${CMAKE_CURRENT_SOURCE_DIR}/linker.ld)
