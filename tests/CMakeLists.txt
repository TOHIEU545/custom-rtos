# Stack and task tests.
add_executable(test_initial_stack stack/initial_stack.c)
target_link_libraries(test_initial_stack PRIVATE
    rtos_kernel rtos_port rtos_platform_stm32f103)
add_executable(test_stack_diagnostics stack/diagnostics.c)
target_link_libraries(test_stack_diagnostics PRIVATE
    rtos_kernel rtos_port rtos_platform_stm32f103)
add_executable(test_first_task task/first_task.c)
target_link_libraries(test_first_task PRIVATE
    rtos_kernel rtos_port rtos_platform_stm32f103)
add_executable(test_task_delay task/delay.c)
target_link_libraries(test_task_delay PRIVATE
    rtos_kernel rtos_port rtos_platform_stm32f103)

# Scheduler tests.
add_executable(test_scheduler_cooperative scheduler/cooperative.c)
target_link_libraries(test_scheduler_cooperative PRIVATE
    rtos_kernel rtos_port rtos_platform_stm32f103)
target_include_directories(test_scheduler_cooperative PRIVATE
    ${PROJECT_SOURCE_DIR}/kernel/src)

add_executable(test_scheduler_preemptive scheduler/preemptive.c)
target_link_libraries(test_scheduler_preemptive PRIVATE
    rtos_kernel rtos_port rtos_platform_stm32f103)
add_executable(test_scheduler_priority scheduler/priority.c)
target_link_libraries(test_scheduler_priority PRIVATE
    rtos_kernel rtos_port rtos_platform_stm32f103)
add_executable(test_scheduler_ready_list_priority scheduler/ready_list_priority.c)
target_link_libraries(test_scheduler_ready_list_priority PRIVATE
    rtos_kernel rtos_port rtos_platform_stm32f103)
add_executable(test_scheduler_ready_list_blocking scheduler/ready_list_blocking.c)
target_link_libraries(test_scheduler_ready_list_blocking PRIVATE
    rtos_kernel rtos_port rtos_platform_stm32f103)
target_include_directories(test_scheduler_ready_list_blocking PRIVATE
    ${PROJECT_SOURCE_DIR}/kernel/src)

# Queue tests.
add_executable(test_queue_fifo queue/fifo.c)
target_link_libraries(test_queue_fifo PRIVATE
    rtos_kernel rtos_port rtos_platform_stm32f103)
add_executable(test_queue_blocking_receive queue/blocking_receive.c)
target_link_libraries(test_queue_blocking_receive PRIVATE
    rtos_kernel rtos_port rtos_platform_stm32f103)
target_include_directories(test_queue_blocking_receive PRIVATE
    ${PROJECT_SOURCE_DIR}/kernel/src)

add_executable(test_queue_blocking_send queue/blocking_send.c)
target_link_libraries(test_queue_blocking_send PRIVATE
    rtos_kernel rtos_port rtos_platform_stm32f103)
target_include_directories(test_queue_blocking_send PRIVATE
    ${PROJECT_SOURCE_DIR}/kernel/src)

add_executable(test_queue_isr queue/queue_isr.c)
target_link_libraries(test_queue_isr PRIVATE
    rtos_kernel rtos_port rtos_platform_stm32f103)

# Semaphore tests.
add_executable(test_semaphore_basic semaphore/basic.c)
target_link_libraries(test_semaphore_basic PRIVATE
    rtos_kernel rtos_port rtos_platform_stm32f103)
add_executable(test_semaphore_blocking semaphore/blocking.c)
target_link_libraries(test_semaphore_blocking PRIVATE
    rtos_kernel rtos_port rtos_platform_stm32f103)
target_include_directories(test_semaphore_blocking PRIVATE
    ${PROJECT_SOURCE_DIR}/kernel/src)

add_executable(test_semaphore_isr semaphore/semaphore_isr.c)
target_link_libraries(test_semaphore_isr PRIVATE
    rtos_kernel rtos_port rtos_platform_stm32f103)

# Full-system tests.
add_executable(test_full_system_stress stress/full_system_stress.c)
target_link_libraries(test_full_system_stress PRIVATE
    rtos_kernel rtos_port rtos_platform_stm32f103)
add_executable(test_integration_full_demo integration/full_demo.c)
target_link_libraries(test_integration_full_demo PRIVATE
    rtos_kernel rtos_port rtos_platform_stm32f103)
