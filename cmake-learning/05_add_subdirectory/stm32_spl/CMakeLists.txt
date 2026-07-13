add_library(stm32_spl STATIC
    "${PROJECT_ROOT}/driver/STM32F10x_StdPeriph_Driver/src/stm32f10x_gpio.c"
    "${PROJECT_ROOT}/driver/STM32F10x_StdPeriph_Driver/src/stm32f10x_rcc.c"
)

target_include_directories(stm32_spl PUBLIC
    "${PROJECT_ROOT}/driver/CMSIS/Include"
    "${PROJECT_ROOT}/driver/CMSIS/Device/ST/STM32F1xx/Include"
    "${PROJECT_ROOT}/driver/STM32F10x_StdPeriph_Driver/inc"
    "${PROJECT_ROOT}/bsp/stm32f103c8/include"
)

target_compile_definitions(stm32_spl PUBLIC
    STM32F10X_MD
    USE_STDPERIPH_DRIVER
)

target_compile_options(stm32_spl PRIVATE
    ${COMMON_C_FLAGS}
)
