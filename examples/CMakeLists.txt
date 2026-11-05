add_executable(example_scheduler_demo scheduler_demo.c)
target_link_libraries(example_scheduler_demo PRIVATE
    rtos_kernel rtos_port rtos_platform_stm32f103)

add_executable(example_ipc_demo ipc_demo.c)
target_link_libraries(example_ipc_demo PRIVATE
    rtos_kernel rtos_port rtos_platform_stm32f103)
