# NTOS - Real Time OS

**ntos** là một RTOS (Real-Time Operating System) nhỏ, mang tính học tập,
được viết cho **STM32F103 / ARM Cortex-M3**. Mục tiêu của dự án không phải
là tạo ra một RTOS production-grade, mà là để hiểu rõ bản chất của:

- Task tĩnh và context switch.
- Scheduler fixed-priority với ready list.
- Blocking (delay, semaphore, queue).
- IPC giữa các task, và giữa ISR với task.

Toàn bộ kernel/port được viết thuần C + một phần assembly tối thiểu cho
context switch, không phụ thuộc heap, không xoá task khi runtime.

## Phần cứng sử dụng

- **MCU:** STM32F103 (ARM Cortex-M3), theo linker script hiện tại:
  256 KB Flash, 64 KB RAM (`bsp/stm32f10x/linker/stm32f10x.ld`).
- **Debug probe:** ST-Link (kết nối qua SWD).
- **Giao tiếp:** USART1, dùng để log/debug qua UART trong các ví dụ.

## Tool để build và debug

| Tool | Vai trò |
|---|---|
| CMake (>= 3.22) | Cấu hình và điều phối build |
| `arm-none-eabi-gcc` / `arm-none-eabi-objcopy` / `arm-none-eabi-size` | Toolchain cross-compile cho Cortex-M3 (`cmake/arm-none-eabi-toolchain.cmake`) |
| OpenOCD | Nạp firmware và mở phiên debug qua ST-Link (`tools/openocd/stm32f103.cfg`) |
| GDB (`gdb-multiarch`) | Debug target qua OpenOCD (`tools/gdb/ntos.gdb`, kết nối `:3333`) |

## Cấu trúc thư mục

```text
ntos/
├── include/ntos/          Public API: ntos.h, ntos_task.h, ntos_sem.h, ntos_queue.h...
├── kernel/                Kernel thuần, không biết CPU cụ thể (task, list, assert).
│   └── internal/          Header nội bộ kernel — chỉ port/ được phép include chéo.
├── port/cortex_m3/        Lớp nối kernel với Cortex-M3: SVC, PendSV, PSP/MSP, ASM.
├── bsp/stm32f10x/         Board support: clock, gpio, uart, exti, startup, linker.
├── cmsis/                 Vendor: CMSIS core + STM32F10x device headers (header-only).
├── examples/              Firmware demo dùng kernel/port (scheduler demo, IPC demo).
├── tests/
│   ├── stack/             Test board thật — initial stack, diagnostics.
│   ├── task/              Test board thật — first task, delay.
│   ├── scheduler/         Test board thật — cooperative/preemptive/priority/ready-list.
│   ├── queue/             Test board thật — queue + queue ISR.
│   ├── semaphore/         Test board thật — semaphore + semaphore ISR.
│   ├── stress/            Test board thật — stress toàn hệ thống.
│   └── integration/       Test board thật — demo tích hợp.
├── docs/                  Tài liệu thiết kế + coding convention.
├── scripts/               build.sh, flash.sh, clean.sh, debug-server.sh, debug.sh.
├── tools/                 Cấu hình OpenOCD, GDB.
└── cmake/                 Toolchain file, compiler options, hàm ntos_add_firmware().
```

## Các bước để build và chạy

Hai nhóm firmware — `examples/` và `tests/` — được bật/tắt bằng hai option
trong `CMakeLists.txt` ở thư mục gốc:

```cmake
option(NTOS_BUILD_EXAMPLES "Build ntos examples" ON)
option(NTOS_BUILD_TESTS "Build tests" OFF)
```

Muốn build nhóm nào thì sửa trực tiếp hai dòng này rồi build lại.

**Build examples (mặc định):**

```cmake
option(NTOS_BUILD_EXAMPLES "Build ntos examples" ON)
option(NTOS_BUILD_TESTS "Build tests" OFF)
```

```sh
# Build một target — chỉ compile lại phần đã đổi
scripts/build.sh example_scheduler_demo

# Build sạch từ đầu
scripts/clean.sh && scripts/build.sh example_scheduler_demo
```

**Build tests:** đổi hai option thành

```cmake
option(NTOS_BUILD_EXAMPLES "Build ntos examples" OFF)
option(NTOS_BUILD_TESTS "Build tests" ON)
```

```sh
scripts/clean.sh && scripts/build.sh test_initial_stack
```

Target firmware mới luôn được khai báo qua `ntos_add_firmware()` trong
`cmake/ntos_firmware.cmake`, không tự gọi `add_executable` tay.

## Flash và debug chương trình

Sau khi đã build ở bước trên, nạp `build/firmware/current.elf` (ELF của lần
build gần nhất) xuống board qua OpenOCD + ST-Link:

```sh
scripts/flash.sh
```

Debug thủ công bằng OpenOCD + `gdb-multiarch` (không phụ thuộc Cortex-Debug hay
`launch.json`), dùng 2 terminal:

```sh
# Terminal 1 — khởi động OpenOCD server (tools/openocd/stm32f103.cfg, port 3333).
# Nếu có gnome-terminal, script tự mở một cửa sổ terminal mới để chạy OpenOCD,
# trả lại ngay terminal hiện tại.
scripts/debug-server.sh

# Terminal 2 (hoặc terminal hiện tại nếu OpenOCD đã chuyển sang cửa sổ khác)
# — attach gdb-multiarch vào build/firmware/current.elf.
scripts/debug.sh
```

`debug.sh` dùng chung command file `tools/gdb/ntos.gdb`: tự connect
`target extended-remote :3333`, `monitor reset halt`, rồi đặt breakpoint tạm
ở `main()` và `continue` — chương trình dừng ngay khi vào `main()`, không tự
đặt thêm breakpoint hay chạy tiếp.

## Tài liệu thiết kế (`docs/`)

- [`docs/architecture.md`](docs/architecture.md)
- [`docs/context_switch.md`](docs/context_switch.md)
- [`docs/ipc.md`](docs/ipc.md)
- [`docs/known_limitations.md`](docs/known_limitations.md)
- [`docs/scheduler.md`](docs/scheduler.md)
- [`docs/task_states.md`](docs/task_states.md)
