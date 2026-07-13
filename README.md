# ntos

**ntos — Nghĩa Trụ Operating System** is a small educational RTOS for STM32F103 / Cortex-M3.

Core learning goals:
1. Save and restore CPU context.
2. Select the next task.
3. Move tasks between READY and BLOCKED states.

## Main directories

- `kernel/`: canonical kernel implementation.
- `port/cortex_m3/`: SVC, PendSV, PSP/MSP and interrupt primitives.
- `bsp/stm32f10x/`: board support for the STM32F103 line (raw register access, no StdPeriph).
- `cmsis/`: vendor CMSIS core and STM32F10x device headers.
- `include/ntos/`: public ntos API.
- `labs/`: bare-metal Cortex-M experiments (no kernel/port).
- `examples/`: applications using the current ntos API.
- `tests/`: host and target tests.
- `docs/`: roadmap, design and learning log.
