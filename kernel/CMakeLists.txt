# Portable ntos kernel code. Architecture-specific code lives under port/.
add_library(ntos_kernel STATIC ntos_kernel.c ntos_task.c ntos_list.c ntos_assert.c)

target_include_directories(ntos_kernel
    PUBLIC ${CMAKE_SOURCE_DIR}/include
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/internal)
target_link_libraries(ntos_kernel PUBLIC ntos_compiler_options cmsis)
