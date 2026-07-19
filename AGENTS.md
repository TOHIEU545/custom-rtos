# Agent instructions for ntos

Canonical implementation:
- `include/ntos/`
- `kernel/`
- `port/cortex_m3/`
- `bsp/stm32f10x/`
- `cmsis/`

Do not inspect `examples/`, `tests/`, or `docs/` unless the task explicitly requires them.
Bare-metal Cortex-M experiments live on the `learning/cortex-m3` branch, not on
main-track — they are not the ntos kernel.
Prefer opening only files directly related to the requested module.

Design constraints:
- Static tasks and stacks.
- Cortex-M3 only.
- SVC starts the first task.
- PendSV is the only context-switch point.
- Tasks use PSP; exceptions use MSP.
- No task deletion or mandatory general-purpose heap.
