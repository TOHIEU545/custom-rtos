# Common Cortex-M3 compile/link flags.
add_library(ntos_compiler_options INTERFACE)

target_compile_options(ntos_compiler_options INTERFACE
    -mcpu=cortex-m3 -mthumb
    -ffunction-sections -fdata-sections -fno-common
    -Wall -Wextra -Werror -Wshadow -Wundef)

target_link_options(ntos_compiler_options INTERFACE
    -mcpu=cortex-m3 -mthumb -Wl,--gc-sections)
