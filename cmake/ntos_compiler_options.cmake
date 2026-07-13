# Common Cortex-M3 compile/link flags.
add_library(ntos_compiler_options INTERFACE)

target_compile_options(ntos_compiler_options INTERFACE
    -mcpu=cortex-m3 -mthumb
    -g -O0
    -ffunction-sections -fdata-sections -fno-common
    -Wall -Wextra -Wshadow -Wundef)

target_link_options(ntos_compiler_options INTERFACE
    -mcpu=cortex-m3 -mthumb -Wl,--gc-sections)
