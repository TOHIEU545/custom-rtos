# Agent instructions for ntos

Canonical implementation:
- `include/ntos/`
- `kernel/`
- `port/cortex_m3/`
- `bsp/stm32f103c8/`

Do not inspect `labs/`, `examples/`, `tests/`, or `docs/` unless the task explicitly requires them.
`labs/` contains isolated Cortex-M experiments and is not the ntos kernel.
Prefer opening only files directly related to the requested module.

Design constraints:
- Static tasks and stacks.
- Cortex-M3 only.
- SVC starts the first task.
- PendSV is the only context-switch point.
- Tasks use PSP; exceptions use MSP.
- No task deletion or mandatory general-purpose heap.
