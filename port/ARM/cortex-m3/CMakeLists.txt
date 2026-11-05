# Cortex-M3 port layer for MyRTOS.
add_library(rtos_port STATIC rtos_port.c)

target_include_directories(rtos_port
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/cmsis/Core/Include
        ${CMAKE_CURRENT_LIST_DIR}/../../../kernel/include
        ${CMAKE_CURRENT_LIST_DIR}/../../../kernel/src)
