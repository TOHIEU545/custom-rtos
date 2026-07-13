# Create one firmware image.
#
# Usage:
#   ntos_add_firmware(name
#       [BARE_METAL]
#       MAIN main.c
#       [SOURCES extra.c handler.S]
#       [CONFIG_DIR path])
#
# BARE_METAL skips the ntos kernel/port.
function(ntos_add_firmware target)
    cmake_parse_arguments(ARG "BARE_METAL" "MAIN;CONFIG_DIR" "SOURCES" ${ARGN})

    if(NOT ARG_MAIN)
        message(FATAL_ERROR "ntos_add_firmware(${target}) requires MAIN <file>")
    endif()

    # Final firmware ELF.
    add_executable(${target} ${ARG_MAIN} ${ARG_SOURCES})

    # Platform gives this target CMSIS, BSP, flags, startup, and linker script.
    target_link_libraries(${target} PRIVATE ntos_platform_stm32f10x)

    # Examples use the OS; labs usually do not.
    if(NOT ARG_BARE_METAL)
        target_link_libraries(${target} PRIVATE ntos_kernel ntos_port)
    endif()

    # Optional per-firmware config directory.
    if(ARG_CONFIG_DIR)
        target_include_directories(${target} PRIVATE ${ARG_CONFIG_DIR})
    endif()

    # Map file for debugging linker output.
    target_link_options(${target} PRIVATE
        -Wl,-Map=${CMAKE_CURRENT_BINARY_DIR}/${target}.map)
endfunction()
