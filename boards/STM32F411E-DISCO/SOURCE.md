# STM32F411E-DISCO board sources

The board-owned startup and linker files were implemented against these
official STMicroelectronics references:

- Startup vector and reset sequence:
  `cmsis-device-f4/Source/Templates/gcc/startup_stm32f411xe.s`
  at commit `9192c7b9df75a142f2027ab266601fe061fc00b3`.
- Memory map and linker section layout:
  `STM32CubeF4/Projects/STM32F411E-Discovery/Demonstrations/STM32CubeIDE/STM32F411VEHX_FLASH.ld`
  at commit `60c8abe0ae88a23bbd1c925358330031e36dd20f`.
- User LED and B1 button mappings:
  `32f411ediscovery-bsp/stm32f411e_discovery.h`
  at commit `c7de631adeb20ecd2097581ab22e88d316a8bdb3`.

The local files use the same device memory sizes and interrupt-vector order,
while following this project's C startup and linker-symbol conventions.
