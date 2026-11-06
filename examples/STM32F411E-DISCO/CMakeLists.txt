add_executable(example_port_validation port_validation.c)
target_link_libraries(example_port_validation PRIVATE
    rtos_kernel rtos_port rtos_platform)
