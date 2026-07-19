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
  Build/flash/debug qua CMake chạy được; code lab + báo cáo chi tiết
  (`cortex_m3_exception_msp_psp_svc_report.md`) không còn nằm trên main-track,
  đã chuyển sang branch `learning/cortex-m3` (xem mục "Learning material" bên dưới).
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
examples/           Firmware có dùng kernel/port (một khi kernel xong).
tests/host/         Test chạy trên host (pure C, không cần board).
tests/target/       Test chạy trên board thật.
docs/               Tài liệu thiết kế + roadmap.
scripts/            build.sh, flash.sh, clean.sh.
tools/               Cấu hình OpenOCD, GDB.
cmake/               Toolchain file, compiler options, hàm ntos_add_firmware().
```

Bare-metal labs (Cortex-M3 exception/MSP-PSP/SVC) và bài tập CMake độc lập
không còn nằm trên main-track — xem mục "Learning material" bên dưới.

Không thư mục nào (trừ một ngoại lệ: `port/` include thẳng `kernel/internal/`
vì context-switch cần thấy layout TCB nội bộ) được mở đường dẫn vào phần
private của thư mục khác. Xem `docs/cmake_flow.md` mục 6 nếu build lỗi liên
quan tới include đó.

## Build / flash / debug

```sh
# Cấu hình + build một target (chỉ compile lại phần đổi)
scripts/build.sh chang02_initial_stack

# Build sạch từ đầu
scripts/clean.sh && scripts/build.sh chang02_initial_stack

# Nạp firmware.elf hiện tại (build/firmware/current.elf) qua OpenOCD
scripts/flash.sh

# Bật thêm examples khi cần (mặc định: NTOS_BUILD_TESTS=ON, NTOS_BUILD_EXAMPLES=OFF)
cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=cmake/arm-none-eabi-toolchain.cmake \
    -DNTOS_BUILD_EXAMPLES=ON -DNTOS_BUILD_TESTS=ON
cmake --build build --target example_basic_tasks
```

Target firmware mới luôn khai báo qua `ntos_add_firmware()` trong
`cmake/ntos_firmware.cmake` (xem `tests/target/CMakeLists.txt` để làm ví dụ),
không tự gọi `add_executable` tay.

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
- Comment trong code của project này ưu tiên viết bằng tiếng Việt khi giải
  thích luồng build/script (xem `scripts/build.sh`, `scripts/flash.sh`).

## Learning material (không nằm trên main-track)

Quá trình học/milestone không cần thiết để hiểu, build, dùng hay test NTOS đã
được tách khỏi nhánh chính, giữ nguyên vẹn ở Git history + 2 branch riêng:

- `learning/cortex-m3` — bare-metal Cortex-M3 labs (exception frame, MSP/PSP,
  SVC) + báo cáo chi tiết, trước đây ở `labs/`.
- `learning/cmake` — bài tập học CMake độc lập, không liên quan STM32F103/ntos,
  trước đây ở `cmake-learning/`.

Không xoá 2 branch này khi dọn dẹp repo.
