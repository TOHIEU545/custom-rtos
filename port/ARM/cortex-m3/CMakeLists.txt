# Cortex-M3 port layer for MyRTOS.
add_library(rtos_arch_options INTERFACE)
target_compile_options(rtos_arch_options INTERFACE
    -mcpu=cortex-m3
    -mthumb)
target_link_options(rtos_arch_options INTERFACE
    -mcpu=cortex-m3
    -mthumb)

add_library(rtos_port STATIC rtos_port.c)

target_include_directories(rtos_port
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/cmsis/Core/Include
        ${CMAKE_CURRENT_LIST_DIR}/../../../kernel/include
        ${CMAKE_CURRENT_LIST_DIR}/../../../kernel/src)

target_link_libraries(rtos_port PUBLIC rtos_arch_options)
