# CLAUDE.md

## Dự án là gì

**ntos** (Nghĩa Trụ Operating System) — một RTOS educational nhỏ cho
STM32F103 / ARM Cortex-M3, mục tiêu là hiểu bản chất task, context switch,
scheduler, blocking và IPC, không phải để dùng production. Tài liệu gốc:

- `docs/ntos_roadmap.md` — lộ trình phát triển theo từng chặng (0–10), có mục lục.
- `docs/ntos_design.md` — "hợp đồng kiến trúc": TCB, state machine, invariant, API draft.
- `docs/cmake_flow.md` — cách CMake thật của project chạy, target nào phụ thuộc target nào.
- `AGENTS.md` — phạm vi code chính thức và các ràng buộc thiết kế cốt lõi.

## Trạng thái thực tế (đọc code trước khi giả định)

Đừng tin vào tên thư mục hay comment cũ — luôn kiểm tra lại code. Tại thời điểm
viết file này:

- **Bare-metal foundation (CHẶNG 0) và exception model (CHẶNG 1): đã xong.**
  Build/flash/debug qua CMake chạy được; `labs/01_exception_frame`,
  `labs/02_msp_psp`, `labs/03_svc` có code + báo cáo chi tiết
  (`labs/cortex_m3_exception_msp_psp_svc_report.md`).
- **Kernel (`kernel/*.c`) và port (`port/cortex_m3/*.c`) hiện toàn bộ là stub**
  (`/* TODO(stage N). */`), chưa có implementation thật: chưa có task, chưa
  scheduler, chưa context switch. `ntos_start()` chỉ là vòng lặp rỗng.
- `include/ntos/*.h` đã khai báo public API (task/sem/queue) nhưng struct TCB
  thật trong code chưa khớp hết với bản thiết kế trong `ntos_design.md`.
- `examples/01`–`07` dùng API tương lai của kernel; chưa chạy được vì kernel
  chưa implement. `tests/host` và `tests/target` phần lớn vẫn là placeholder.
- Không có git tag nào trong repo. Xem `docs/ntos_roadmap.md#b-bảng-tiến-độ`
  để biết trạng thái từng chặng, và cập nhật bảng đó khi code thay đổi.

## Cấu trúc thư mục

```text
include/ntos/       Public API: ntos.h, ntos_task.h, ntos_sem.h, ntos_queue.h...
kernel/             Kernel thuần, không biết CPU cụ thể (task, list, assert).
kernel/internal/    Header nội bộ kernel — chỉ port/ được phép include chéo.
port/cortex_m3/     Lớp nối kernel với Cortex-M3: SVC, PendSV, PSP/MSP, ASM.
bsp/stm32f10x/       Board support: clock, gpio, uart, exti, startup, linker.
cmsis/              Vendor: CMSIS core + STM32F10x device headers (header-only).
labs/               Firmware bare-metal thuần, KHÔNG dùng kernel/port.
examples/           Firmware có dùng kernel/port (một khi kernel xong).
tests/host/         Test chạy trên host (pure C, không cần board).
tests/target/       Test chạy trên board thật.
docs/               Tài liệu thiết kế + roadmap.
scripts/            build.sh, flash.sh, clean.sh.
tools/               Cấu hình OpenOCD, GDB.
cmake/               Toolchain file, compiler options, hàm ntos_add_firmware().
cmake-learning/      Bài tập học CMake độc lập, KHÔNG liên quan tới ntos kernel.
```

Không thư mục nào (trừ một ngoại lệ: `port/` include thẳng `kernel/internal/`
vì context-switch cần thấy layout TCB nội bộ) được mở đường dẫn vào phần
private của thư mục khác. Xem `docs/cmake_flow.md` mục 6 nếu build lỗi liên
quan tới include đó.

## Build / flash / debug

```sh
# Cấu hình + build một target (chỉ compile lại phần đổi)
scripts/build.sh lab_exception_frame

# Build sạch từ đầu
scripts/clean.sh && scripts/build.sh lab_exception_frame

# Nạp firmware.elf hiện tại (build/firmware/current.elf) qua OpenOCD
scripts/flash.sh

# Bật thêm examples/tests khi cần (mặc định chỉ labs được build)
cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=cmake/arm-none-eabi-toolchain.cmake \
    -DNTOS_BUILD_EXAMPLES=ON -DNTOS_BUILD_TESTS=ON
cmake --build build --target example_basic_tasks
```

Target firmware mới luôn khai báo qua `ntos_add_firmware()` trong
`cmake/ntos_firmware.cmake` (xem `labs/CMakeLists.txt` để làm ví dụ), không tự
gọi `add_executable` tay.

## Ràng buộc thiết kế cốt lõi (đọc kỹ trước khi sửa kernel/port)

- Task tĩnh (static), stack tĩnh — không heap bắt buộc, không xóa task.
- Chỉ Cortex-M3. SVC khởi động task đầu tiên. **PendSV là nơi duy nhất** thực
  hiện context switch — mọi chỗ khác chỉ được *request* reschedule.
- Task dùng PSP; exception/ISR dùng MSP.
- ISR chỉ dùng API hậu tố `_from_isr`, không block, không gọi heap.
- Ưu tiên: đúng bản chất > dễ hiểu > dễ debug bằng GDB > không phụ thuộc heap.
  Không tối ưu cycle, không thêm feature "vì RTOS khác có".
- Mọi thay đổi kiến trúc phải phản ánh lại vào `docs/ntos_design.md`, mọi
  tiến độ chặng phải phản ánh lại vào `docs/ntos_roadmap.md`.

## Khi làm việc trong repo này

- Trước một PR/commit kernel lớn, tự trả lời (theo `ntos_design.md` mục 29):
  task state nào thay đổi, task rời/vào cấu trúc nào, có cần PendSV không, có
  gọi được từ ISR không, invariant nào mới, test nào chứng minh.
- `labs/` là bài tập bare-metal cô lập để học exception model, không phải code
  RTOS — đừng lẫn nó với `kernel/`/`port/`.
- `cmake-learning/` là scratch học CMake riêng, có build artifact commit sẵn
  trong đó (thư mục `build/` bên trong nó không nằm trong `.gitignore` gốc do
  cấu trúc lồng); không cần động vào khi làm việc trên ntos kernel.
- Comment trong code của project này ưu tiên viết bằng tiếng Việt khi giải
  thích luồng build/script (xem `scripts/build.sh`, `scripts/flash.sh`).
