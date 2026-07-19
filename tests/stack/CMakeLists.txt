ntos_add_firmware(test_initial_stack
    MAIN ${CMAKE_CURRENT_SOURCE_DIR}/initial_stack.c
)

ntos_add_firmware(test_stack_diagnostics
    MAIN ${CMAKE_CURRENT_SOURCE_DIR}/diagnostics.c
)
