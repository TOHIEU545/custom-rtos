set(TARGET_NAME lesson_05)

add_executable(${TARGET_NAME}
    main.c
    "${PROJECT_ROOT}/bsp/stm32f103c8/startup/startup_stm32f103.c"
    "${PROJECT_ROOT}/bsp/stm32f103c8/system/system_stm32f10x.c"
)

target_link_libraries(${TARGET_NAME} PRIVATE
    led_driver
)

target_compile_options(${TARGET_NAME} PRIVATE
    ${COMMON_C_FLAGS}
)

target_link_options(${TARGET_NAME} PRIVATE
    ${MCU_FLAGS}
    -nostartfiles
    -Wl,--gc-sections
    -Wl,-Map=${TARGET_NAME}.map
    -T${PROJECT_ROOT}/bsp/stm32f103c8/linker/stm32f103c8.ld
)

set_target_properties(${TARGET_NAME} PROPERTIES
    SUFFIX ".elf"
)

add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O ihex $<TARGET_FILE:${TARGET_NAME}> ${TARGET_NAME}.hex
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${TARGET_NAME}> ${TARGET_NAME}.bin
    COMMAND ${CMAKE_SIZE} $<TARGET_FILE:${TARGET_NAME}>
    COMMAND ${CMAKE_OBJDUMP} -h $<TARGET_FILE:${TARGET_NAME}>
)
