# CMake trong ntos — luồng thật của project

Tài liệu này là phần tiếp theo của [`cmake_fundamentals.md`](cmake_fundamentals.md).
File đó nói về CMake nói chung; file này áp các khái niệm đó vào đúng
`CMakeLists.txt` thật của `ntos`, để sau này quay lại đọc code là nhớ ra ngay
hệ thống đang làm gì, target nào phụ thuộc target nào, lệnh nào chạy lúc nào.

---

## 1. Sơ đồ thư mục liên quan tới build

```
CMakeLists.txt                 <- root: option(), include(), add_subdirectory()
cmake/
├── arm-none-eabi-toolchain.cmake   <- chọn compiler ARM (đọc TRƯỚC project())
├── ntos_compiler_options.cmake     <- cờ compile/link chung (-mcpu, -Wall...)
└── ntos_firmware.cmake             <- hàm ntos_add_firmware(), điểm nối mọi lab/example

cmsis/                          <- vendor: CMSIS core + STM32F10x device headers (header-only)
bsp/stm32f10x/                  <- board support: init clock/gpio/uart/exti + linker script
kernel/                         <- kernel thuần, không biết gì về CPU cụ thể
port/cortex_m3/                 <- lớp nối kernel với Cortex-M3 (SVC, PendSV, PSP/MSP)
labs/                           <- firmware bare-metal, không dùng kernel/port
examples/                       <- firmware có dùng kernel/port
tests/                          <- chỗ để dành, phần lớn còn là placeholder
```

Mỗi thư mục có `CMakeLists.txt` chỉ khai báo phần của chính nó — không thư mục
nào (trừ một ngoại lệ ở §6) mở đường dẫn vào phần private của thư mục khác.

---

## 2. Thứ tự CMake thật sự chạy khi bạn configure

```
cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=cmake/arm-none-eabi-toolchain.cmake
```

1. **Trước `project()`**: CMake đọc `cmake/arm-none-eabi-toolchain.cmake` vì
   được truyền qua `-DCMAKE_TOOLCHAIN_FILE`. File này set
   `CMAKE_C_COMPILER=arm-none-eabi-gcc` — nếu không có bước này, CMake sẽ dùng
   compiler host (gcc thường) và toàn bộ code Cortex-M3 sẽ không biên dịch
   được (hoặc biên dịch sai kiến trúc).
2. `project(ntos LANGUAGES C ASM)` — khai báo ngôn ngữ, kiểm tra compiler.
3. `option(NTOS_BUILD_LABS ON)`, `option(NTOS_BUILD_EXAMPLES OFF)`,
   `option(NTOS_BUILD_TESTS OFF)` — 3 công tắc bật/tắt cả nhóm firmware.
   Mặc định: chỉ build labs.
4. `include(cmake/ntos_compiler_options.cmake)` và
   `include(cmake/ntos_firmware.cmake)` — **`include()` khác `add_subdirectory()`**:
   nó chỉ nạp các lệnh `add_library()`/`function()` vào scope hiện tại (root),
   không tạo thư mục con trong build-tree, không đổi
   `CMAKE_CURRENT_SOURCE_DIR`.
5. `add_subdirectory(cmsis)` → chạy `cmsis/CMakeLists.txt`.
6. `add_subdirectory(bsp/stm32f10x)` → chạy `bsp/stm32f10x/CMakeLists.txt`
   (target này link `cmsis`, nên phải add sau bước 5).
7. Nếu `NTOS_BUILD_EXAMPLES=ON`: `add_subdirectory(kernel)` rồi
   `add_subdirectory(port/cortex_m3)`.
8. Nếu `NTOS_BUILD_LABS=ON` (mặc định có): `add_subdirectory(labs)`.
9. Nếu `NTOS_BUILD_EXAMPLES=ON`: `add_subdirectory(examples)`.
10. Nếu `NTOS_BUILD_TESTS=ON`: `add_subdirectory(tests)`.

Thứ tự add_subdirectory ở trên **phải** tôn trọng chiều phụ thuộc: một target
chỉ có thể `target_link_libraries` một target đã được `add_library`/
`add_executable` trước đó (hoặc ít nhất được biết tới trong cùng lượt
configure — CMake thật ra cho phép khai báo library ở subdirectory nào cũng
được miễn tên khớp, nhưng để dễ đọc, project này add theo đúng chiều phụ
thuộc: vendor → platform → kernel/port → firmware).

---

## 3. Cây phụ thuộc target thật sự

```
ntos_compiler_options (INTERFACE)        cmsis (INTERFACE)
      -mcpu=cortex-m3 -Wall...           CMSIS headers + STM32F10X_MD
            ^   ^   ^                          ^   ^   ^
            |   |   |                          |   |   |
            |   |   +------ ntos_bsp (STATIC) -+   |   |
            |   |                                   |   |
            |   +---------- ntos_kernel (STATIC) ---+   |
            |                                            |
            +-------------- ntos_port (STATIC) ----------+

ntos_platform_stm32f10x (INTERFACE)
  = ntos_compiler_options + ntos_bsp + cmsis
  + linker script (-T bsp/stm32f10x/linker/stm32f10x.ld)

add_executable(lab_xxx / example_xxx)     <- sinh ra bởi ntos_add_firmware()
  PRIVATE ntos_platform_stm32f10x
  PRIVATE ntos_kernel + ntos_port          (chỉ khi KHÔNG có cờ BARE_METAL)
```

Ba loại target xuất hiện trong project:

| Loại | Ví dụ | Sinh ra file gì | Khi nào build |
|---|---|---|---|
| `INTERFACE` | `ntos_compiler_options`, `cmsis`, `ntos_platform_stm32f10x` | **Không file nào cả** — chỉ là túi thuộc tính (include dirs, defines, link options) | Không bao giờ tự build, chỉ propagate khi có ai link vào |
| `STATIC` | `ntos_bsp`, `ntos_kernel`, `ntos_port` | `.a` (qua `ar`, không qua linker thật) | Khi có target nào đó (trực tiếp/gián tiếp) cần nó |
| `EXECUTABLE` | `lab_exception_frame`, `example_basic_tasks`,... | `.elf` (qua `gcc` với đầy đủ cờ link, bao gồm `-T`) | Khi bạn `cmake --build --target <tên>` đúng tên đó |

**Chỗ hay hiểu lầm nhất**: `target_link_options(ntos_platform_stm32f10x INTERFACE -T...)`
ở `bsp/stm32f10x/CMakeLists.txt` **không link gì cả tại thời điểm đó**. Nó chỉ
gắn nhãn "-T này thuộc về platform". Cờ `-T` chỉ thật sự xuất hiện trong lệnh
`gcc` khi CMake build một `add_executable()` có link (trực tiếp hoặc gián
tiếp qua chuỗi) tới `ntos_platform_stm32f10x`. Build `ntos_bsp` một mình chỉ
gọi `ar`, không gọi `gcc -T` gì hết.

---

## 4. `ntos_add_firmware()` — điểm nối duy nhất

Định nghĩa ở `cmake/ntos_firmware.cmake`. Mọi lab/example đều đi qua đây, thay
vì tự gọi `add_executable` + `target_link_libraries` tay:

```cmake
ntos_add_firmware(lab_exception_frame
    BARE_METAL                          # bỏ qua ntos_kernel/ntos_port
    MAIN 01_exception_frame/main.c
    SOURCES extra.c handler.S           # optional
    CONFIG_DIR path/to/config           # optional, thêm include riêng cho target
)
```

Bên trong, tóm gọn:

```cmake
add_executable(${target} ${ARG_MAIN} ${ARG_SOURCES})
target_link_libraries(${target} PRIVATE ntos_platform_stm32f10x)
if(NOT ARG_BARE_METAL)
    target_link_libraries(${target} PRIVATE ntos_kernel ntos_port)
endif()
target_link_options(${target} PRIVATE -Wl,-Map=.../${target}.map)
```

`add_executable(${target} ...)` chính là dòng **duy nhất** biến chuỗi
`"lab_exception_frame"` thành một cái tên CMake/make thật sự biết tới — đây là
chỗ để tra nếu bạn truyền `--target <tên gì đó>`

---

## 5. Build-tree mirror source-tree

`add_subdirectory(X)` không đụng vào source tree. Nó khiến CMake tạo một thư
mục `X/` **song song bên trong thư mục build** (`build/X/`), chứa
`Makefile`, `CMakeFiles/` và output (`.o`, `.a`, `.elf`) của target khai báo
trong `X/CMakeLists.txt`. Ví dụ sau khi configure với labs bật:

```
build/labs/
├── Makefile
├── CMakeFiles/
│   ├── lab_exception_frame.dir/   <- .o trung gian
│   ├── lab_msp_psp.dir/
│   ├── lab_svc.dir/
│   └── lab_pendsv.dir/
├── lab_exception_frame            <- ELF, không đuôi mở rộng
├── lab_msp_psp
├── lab_svc
└── lab_pendsv
```

Đây cũng là lý do `scripts/build.sh` tìm ELF bằng
`find build -type f -name "$TARGET"`: tên file trùng khớp tên target.

---

## 6. Một chỗ phá lệ, biết trước để không bất ngờ

`port/cortex_m3/CMakeLists.txt`:

```cmake
target_include_directories(ntos_port
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
    PRIVATE ${CMAKE_SOURCE_DIR}/kernel/internal)
```

`kernel/internal/` được `kernel/CMakeLists.txt` khai là `PRIVATE` — nghĩa là
chủ đích chỉ kernel tự dùng. Nhưng `port` vẫn include thẳng đường dẫn đó, vì
context-switch cần thấy layout TCB nội bộ (không có cách nào tách bạch hoàn
toàn port khỏi kernel — đây là kiểu ràng buộc chặt vốn dĩ tự nhiên giữa
kernel/port trong mọi RTOS thật, không riêng ntos). Đây là **nơi duy nhất**
một module tự ý mở đường dẫn sang phần private của module khác thay vì đi qua
`target_link_libraries`. Nếu sau này thấy build lỗi include liên quan tới
`kernel/internal`, hoặc muốn refactor port sang kiến trúc khác, đây là chỗ
đầu tiên cần soi lại.

---

## 7. Cheat sheet lệnh dùng hàng ngày

```sh
# Build (incremental — chỉ compile lại phần đổi)
scripts/build.sh lab_exception_frame

# Build sạch hoàn toàn (xoá build/ trước)
scripts/clean.sh && scripts/build.sh lab_exception_frame

# Bật thêm examples/tests khi cần
cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=cmake/arm-none-eabi-toolchain.cmake \
    -DNTOS_BUILD_EXAMPLES=ON -DNTOS_BUILD_TESTS=ON
cmake --build build --target example_basic_tasks

# Xem lệnh compile/link thật sự CMake sinh ra cho 1 file/target
cmake --build build --target <tên> --verbose
```

---
