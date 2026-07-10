add_library(ntos_port STATIC ntos_port.c ntos_port_asm.S)
target_include_directories(ntos_port PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} PRIVATE ${CMAKE_SOURCE_DIR}/kernel/internal)
target_link_libraries(ntos_port PUBLIC ntos_compiler_options cmsis)
