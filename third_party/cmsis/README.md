# CMSIS layout

Add vendor files here:

```text
CMSIS/Core/Include/
Device/ST/STM32F1xx/Include/
```

Expected headers include `core_cm3.h`, `cmsis_gcc.h`, `stm32f1xx.h`, `stm32f103xb.h`, and `system_stm32f1xx.h`.
Do not edit vendor headers. Copy startup/system templates into the project BSP before modifying them.
