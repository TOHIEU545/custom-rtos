function(ntos_add_firmware target)
    cmake_parse_arguments(ARG "" "MAIN;CONFIG_DIR" "SOURCES" ${ARGN})
    add_executable(${target} ${ARG_MAIN} ${ARG_SOURCES})
    target_link_libraries(${target} PRIVATE ntos_compiler_options ntos_kernel ntos_port ntos_bsp cmsis)
    if(ARG_CONFIG_DIR)
        target_include_directories(${target} PRIVATE ${ARG_CONFIG_DIR})
    endif()
    target_link_options(${target} PRIVATE
        -T${CMAKE_SOURCE_DIR}/bsp/stm32f103c8/linker/stm32f103c8.ld
        -Wl,-Map=${target}.map)
endfunction()
