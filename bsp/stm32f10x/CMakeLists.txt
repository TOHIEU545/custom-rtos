add_library(ntos_bsp STATIC
    src/board.c src/clock.c src/gpio.c src/uart.c src/exti.c
    system/system_stm32f103.c startup/startup_stm32f103.c)
target_include_directories(ntos_bsp PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/internal)
target_link_libraries(ntos_bsp PUBLIC ntos_compiler_options cmsis)
