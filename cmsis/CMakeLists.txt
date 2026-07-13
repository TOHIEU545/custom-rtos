# Vendor headers: CMSIS core + STM32F10x device register definitions.
# ntos accesses peripherals by register directly, so only these headers
# are needed (no STM32F10x StdPeriph driver library).
add_library(cmsis INTERFACE)
target_include_directories(cmsis INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/Include
    ${CMAKE_CURRENT_SOURCE_DIR}/Device/ST/STM32F1xx/Include)
target_compile_definitions(cmsis INTERFACE STM32F10X_MD)
