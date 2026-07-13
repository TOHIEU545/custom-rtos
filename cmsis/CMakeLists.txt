# Vendor headers: CMSIS core + STM32F10x device register definitions.
# ntos accesses peripherals by register, not through STM32F10x_StdPeriph_Driver,
# so only the CMSIS include paths are exposed here.
add_library(cmsis INTERFACE)
target_include_directories(cmsis INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/CMSIS/Include
    ${CMAKE_CURRENT_SOURCE_DIR}/CMSIS/Device/ST/STM32F1xx/Include)
target_compile_definitions(cmsis INTERFACE STM32F10X_MD)
