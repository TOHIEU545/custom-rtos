# ntos examples use the kernel and Cortex-M3 port. Disabled by default from
# the root CMakeLists.txt (NTOS_BUILD_EXAMPLES=OFF); enable explicitly to build.
add_subdirectory(01_scheduler_demo)
add_subdirectory(02_ipc_demo)
