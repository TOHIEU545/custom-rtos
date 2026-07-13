add_library(led_driver STATIC
    src/led.c
)

target_include_directories(led_driver PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/include
)

target_compile_options(led_driver PRIVATE
    ${COMMON_C_FLAGS}
)

target_link_libraries(led_driver PUBLIC
    stm32_spl
)
