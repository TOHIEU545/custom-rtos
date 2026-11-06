add_executable(example_blink_led blink_led.c)
target_link_libraries(example_blink_led PRIVATE
    rtos_kernel rtos_port rtos_platform)

add_executable(example_scheduler_demo scheduler_demo.c)
target_link_libraries(example_scheduler_demo PRIVATE
    rtos_kernel rtos_port rtos_platform)

add_executable(example_ipc_demo ipc_demo.c)
target_link_libraries(example_ipc_demo PRIVATE
    rtos_kernel rtos_port rtos_platform)
