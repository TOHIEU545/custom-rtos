# MyRTOS - Real Time OS

**MyRTOS** là một RTOS (Real-Time Operating System) nhỏ, mang tính học tập,
được viết cho **STM32F103C8T6 / ARM Cortex-M3**. Mục tiêu của dự án không phải
là tạo ra một RTOS production-grade, mà là để hiểu rõ bản chất của:

- Task tĩnh và context switch.
- Scheduler fixed-priority với ready list.
- Blocking (delay, semaphore, queue).
- IPC giữa các task, và giữa ISR với task.

Toàn bộ kernel/port được viết thuần C + một phần assembly tối thiểu cho
context switch, không phụ thuộc heap, không xoá task khi runtime.

## Phần cứng sử dụng

- **MCU:** STM32F103C8T6 (ARM Cortex-M3), theo linker script hiện tại:
  256 KB Flash, 64 KB RAM (`boards/STM32F103C8T6/linker.ld`).
- **Debug probe:** ST-Link (kết nối qua SWD).
- **Giao tiếp:** USART1, dùng để log/debug qua UART trong các ví dụ.

## Tool để build và debug

| Tool | Vai trò |
|---|---|
| CMake (>= 3.22) | Cấu hình và điều phối build |
| `arm-none-eabi-gcc` / `arm-none-eabi-objcopy` / `arm-none-eabi-size` | Toolchain cross-compile cho Cortex-M3 (`cmake/arm-none-eabi-toolchain.cmake`) |
| OpenOCD | Nạp firmware và mở phiên debug qua ST-Link (`tools/openocd/stm32f103c8t6.cfg`) |
| GDB (`gdb-multiarch`) | Debug target qua OpenOCD (`tools/gdb/my_rtos.gdb`, kết nối `:3333`) |

## Cấu trúc thư mục

```text
my_rtos/
├── kernel/                Kernel, chia cặp header/source theo từng subsystem.
│   ├── include/           Public API: core, task, list, stack, semaphore, queue.
│   └── src/               Implementation tương ứng và kernel_internal.h.
├── port/ARM/cortex-m3/    Cortex-M3 port và bản CMSIS-Core tối thiểu của Arm.
├── boards/STM32F103C8T6/  BSP, startup, linker, CMSIS-Core và ST device files.
├── examples/              Firmware demo được tách theo từng board.
│   └── STM32F103C8T6/     Các example dành riêng cho board STM32F103C8T6.
├── docs/                  Tài liệu thiết kế + coding convention.
├── scripts/               build.sh, flash.sh, clean.sh, debug-server.sh, debug.sh.
├── tools/                 Cấu hình OpenOCD, GDB.
└── cmake/                 Chỉ chứa ARM toolchain file.
```

## Các bước để build và chạy

Các firmware trong `examples/` được bật/tắt bằng option trong
`CMakeLists.txt` ở thư mục gốc:

```cmake
option(RTOS_BUILD_EXAMPLES "Build MyRTOS examples" ON)
```

**Build một example:**

```sh
# Build một target — chỉ compile lại phần đã đổi
scripts/build.sh example_scheduler_demo

# Build sạch từ đầu
scripts/clean.sh && scripts/build.sh example_scheduler_demo
```

Mỗi firmware được khai báo trực tiếp bằng `add_executable()` và
`target_link_libraries()` trong `examples/STM32F103C8T6/CMakeLists.txt`.
Project cố ý không bọc các lệnh này trong helper riêng để luồng build dễ đọc
với một project học tập.

## Flash và debug chương trình

Sau khi đã build ở bước trên, nạp `build/firmware/current.elf` (ELF của lần
build gần nhất) xuống board qua OpenOCD + ST-Link:

```sh
scripts/flash.sh
```

Debug thủ công bằng OpenOCD + `gdb-multiarch` (không phụ thuộc Cortex-Debug hay
`launch.json`), dùng 2 terminal:

```sh
# Terminal 1 — khởi động OpenOCD server (tools/openocd/stm32f103c8t6.cfg, port 3333).
# Nếu có gnome-terminal, script tự mở một cửa sổ terminal mới để chạy OpenOCD,
# trả lại ngay terminal hiện tại.
scripts/debug-server.sh

# Terminal 2 (hoặc terminal hiện tại nếu OpenOCD đã chuyển sang cửa sổ khác)
# — attach gdb-multiarch vào build/firmware/current.elf.
scripts/debug.sh
```

`debug.sh` dùng chung command file `tools/gdb/my_rtos.gdb`: tự connect
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
