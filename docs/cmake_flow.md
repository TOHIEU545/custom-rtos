# CMake trong MyRTOS

Build system được giữ đơn giản để có thể nhìn trực tiếp từ target firmware về
source và library mà nó sử dụng. Project không dùng custom CMake function,
object library hay package export.

## 1. Các file liên quan

```text
CMakeLists.txt                       Cấu hình chung và chọn module
cmake/arm-none-eabi-toolchain.cmake  Chọn ARM GCC toolchain
kernel/CMakeLists.txt                Portable kernel library
port/ARM/cortex-m3/CMakeLists.txt    Cortex-M3 port library
boards/STM32F103/CMakeLists.txt      BSP và linker configuration
examples/CMakeLists.txt              Hai example firmware
tests/CMakeLists.txt                 Các test firmware chạy trên board
```

Mỗi module chính có một `CMakeLists.txt`. Không có CMake file con trong từng
test hoặc example.

## 2. Luồng configure

```sh
cmake -S . -B build \
    -DCMAKE_TOOLCHAIN_FILE=cmake/arm-none-eabi-toolchain.cmake \
    -DRTOS_BUILD_EXAMPLES=ON \
    -DRTOS_BUILD_TESTS=ON
```

Thứ tự xử lý:

1. Toolchain file chọn `arm-none-eabi-gcc` trước khi `project()` chạy.
2. Root `CMakeLists.txt` đặt chuẩn C11 và cờ chung cho Cortex-M3.
3. CMake thêm board STM32F103; board tự chứa CMSIS-Core và ST device files.
4. Nếu examples hoặc tests được bật, CMake thêm Cortex-M3 port rồi kernel.
5. CMake thêm các executable trong nhóm đã bật.

Hai option chỉ giúp giảm số target khi cần:

```cmake
option(RTOS_BUILD_EXAMPLES "Build MyRTOS examples" ON)
option(RTOS_BUILD_TESTS "Build MyRTOS tests" OFF)
```

`scripts/build.sh` luôn bật cả hai để nhận được tên target example hoặc test.

## 3. Các target

```text
rtos_port (STATIC)
  └── rtos_kernel (STATIC)

rtos_bsp + linker.ld
  └── rtos_platform_stm32f103 (INTERFACE)

firmware (EXECUTABLE)
  ├── rtos_kernel
  ├── rtos_port
  └── rtos_platform_stm32f103
```

- `rtos_kernel` build sáu source trong `kernel/src/`: `core`, `list`, `task`,
  `stack`, `semaphore` và `queue`.
- `rtos_port` chứa phần phụ thuộc Cortex-M3 và CMSIS-Core riêng của port.
- `rtos_bsp` chứa BSP/startup, CMSIS-Core riêng của board và ST device files.
- `rtos_platform_stm32f103` gom BSP và linker script cho firmware.

`INTERFACE` target không tạo file binary; nó chỉ truyền include directory,
define hoặc link option cho target sử dụng nó. `STATIC` target tạo library
`.a`. Mỗi `EXECUTABLE` tạo một ARM ELF.

## 4. Khai báo firmware

Firmware được khai báo bằng lệnh CMake cơ bản:

```cmake
add_executable(example_scheduler_demo scheduler_demo.c)

target_link_libraries(example_scheduler_demo PRIVATE
    rtos_kernel
    rtos_port
    rtos_platform_stm32f103)
```

Các dòng này được lặp lại có chủ ý. Với quy mô hiện tại, chúng rõ ràng hơn một
helper function ẩn các bước tạo và link target.

## 5. Build hằng ngày

```sh
# Cách ngắn nhất: configure, build target và lưu ELF hiện tại.
scripts/build.sh example_scheduler_demo
scripts/build.sh test_initial_stack

# Hoặc dùng CMake trực tiếp.
cmake --build build --target example_ipc_demo

# Xem lệnh compiler/linker thật.
cmake --build build --target example_ipc_demo --verbose
```

CMake build incremental: khi tái sử dụng thư mục `build/`, chỉ source bị thay
đổi và các target phụ thuộc mới được biên dịch/link lại.

## 6. Thêm firmware mới

1. Thêm một source vào `examples/` hoặc thư mục tính năng tương ứng trong
   `tests/`.
2. Thêm `add_executable()` vào CMake file của nhóm đó.
3. Link ba target `rtos_kernel`, `rtos_port` và
   `rtos_platform_stm32f103`.
4. Build bằng đúng tên executable vừa khai báo.

Chỉ nên tạo helper CMake khi số target và thuộc tính lặp tăng đủ lớn để việc
duy trì trực tiếp thật sự khó hơn việc đọc helper.
