# Portable MyRTOS kernel code. Architecture-specific code lives under port/.
add_library(rtos_kernel STATIC
    src/core.c
    src/list.c
    src/task.c
    src/stack.c
    src/semaphore.c
    src/queue.c)

target_include_directories(rtos_kernel
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_link_libraries(rtos_kernel PRIVATE rtos_port)
