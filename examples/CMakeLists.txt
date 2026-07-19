# ntos examples use the kernel and Cortex-M3 port. Disabled by default from
# the root CMakeLists.txt (NTOS_BUILD_EXAMPLES=OFF); enable explicitly to build.
add_subdirectory(01_basic_tasks)
add_subdirectory(02_preemptive)
add_subdirectory(03_priority)
add_subdirectory(04_delay)
add_subdirectory(05_semaphore_button)
add_subdirectory(06_uart_queue)
add_subdirectory(07_full_demo)
