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
boards/STM32F103C8T6/CMakeLists.txt  BSP và linker configuration
examples/STM32F103C8T6/CMakeLists.txt
                                     Example firmware của board STM32F103C8T6
```

Mỗi module chính có một `CMakeLists.txt`. Không có CMake file con trong từng
example.

## 2. Luồng configure

```sh
cmake -S . -B build \
    -DCMAKE_TOOLCHAIN_FILE=cmake/arm-none-eabi-toolchain.cmake \
    -DRTOS_BOARD=STM32F103C8T6 \
    -DRTOS_BUILD_EXAMPLES=ON
```

Thứ tự xử lý:

1. Toolchain file chọn `arm-none-eabi-gcc` trước khi `project()` chạy.
2. Root `CMakeLists.txt` đặt chuẩn C11, các warning chung và chọn `RTOS_BOARD`.
3. Board STM32F103C8T6 chọn port Cortex-M3 ở root CMake.
4. Port cung cấp `-mcpu=cortex-m3 -mthumb`; board cung cấp
   `STM32F10X_HD`, startup và linker script.
5. Kernel portable được build mà không chứa cờ Cortex-M hoặc STM32 riêng.
6. Nếu examples được bật, CMake thêm `examples/${RTOS_BOARD}` và các
   executable của đúng board đang chọn.

Option sau giúp bỏ các target example khi chỉ muốn cấu hình platform:

```cmake
option(RTOS_BUILD_EXAMPLES "Build MyRTOS examples" ON)
```

## 3. Các target

```text
rtos_arch_options (INTERFACE: -mcpu=cortex-m3 -mthumb)
  └── rtos_port (STATIC)
        └── rtos_kernel (STATIC)

rtos_bsp + linker.ld
  └── rtos_platform (INTERFACE)

firmware (EXECUTABLE)
  ├── rtos_kernel
  ├── rtos_port
  └── rtos_platform
```

- `rtos_kernel` build sáu source trong `kernel/src/`: `core`, `list`, `task`,
  `stack`, `semaphore` và `queue`.
- `rtos_arch_options` truyền cờ compile/link thuộc kiến trúc Cortex-M3.
- `rtos_port` chứa phần phụ thuộc Cortex-M3 và CMSIS-Core riêng của port.
- `rtos_bsp` chứa BSP/startup, CMSIS-Core riêng của board và ST device files.
- `rtos_platform` gom BSP, kiến trúc và linker script của board đang chọn.

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
    rtos_platform)
```

Các dòng này được lặp lại có chủ ý. Với quy mô hiện tại, chúng rõ ràng hơn một
helper function ẩn các bước tạo và link target.

## 5. Build hằng ngày

```sh
# Cách ngắn nhất: configure, build target và lưu ELF hiện tại.
scripts/build.sh example_scheduler_demo

# Chọn board tường minh; mỗi board dùng một cây build riêng.
scripts/build.sh example_scheduler_demo STM32F103C8T6

# Hoặc dùng CMake trực tiếp.
cmake --build build --target example_ipc_demo

# Xem lệnh compiler/linker thật.
cmake --build build --target example_ipc_demo --verbose
```

CMake build incremental: khi tái sử dụng thư mục `build/`, chỉ source bị thay
đổi và các target phụ thuộc mới được biên dịch/link lại.

## 6. Thêm firmware mới

1. Thêm source vào `examples/<board>/`.
2. Thêm `add_executable()` vào `examples/<board>/CMakeLists.txt`.
3. Link ba target `rtos_kernel`, `rtos_port` và
   `rtos_platform`.
4. Build bằng đúng tên executable vừa khai báo.

Chỉ nên tạo helper CMake khi số target và thuộc tính lặp tăng đủ lớn để việc
duy trì trực tiếp thật sự khó hơn việc đọc helper.
