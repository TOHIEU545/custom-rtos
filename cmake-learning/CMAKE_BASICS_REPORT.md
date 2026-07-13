# CMake Basics Report

Ghi chú này tóm tắt các ý căn bản nhất khi học CMake qua các bài trong thư mục `cmake-learning`.

Mục tiêu không phải nhớ lệnh, mà là hiểu CMake đang làm gì và vì sao cần từng phần.

## 1. CMake là gì?

CMake không trực tiếp compile code.

CMake là công cụ sinh ra hệ build khác, ví dụ:

```text
CMakeLists.txt -> Makefile/Ninja files -> compiler/linker -> .elf/.a/.bin
```

Khi chạy:

```bash
cmake -S . -B build
```

CMake đọc `CMakeLists.txt` và tạo file build trong thư mục `build`.

Khi chạy:

```bash
cmake --build build
```

CMake gọi tool build thật, ví dụ `make` hoặc `ninja`, để compile/link.

## 2. Configure và build khác nhau thế nào?

Configure:

```bash
cmake -S . -B build
```

Nhiệm vụ:

```text
đọc CMakeLists.txt
kiểm tra compiler
tạo Makefile/Ninja
lưu cache cấu hình
```

Build:

```bash
cmake --build build
```

Nhiệm vụ:

```text
compile .c -> .o
đóng gói .o -> .a nếu là static library
link .o + .a -> .elf nếu là firmware
tạo .hex/.bin nếu có custom command
```

## 3. Target là khái niệm quan trọng nhất

Trong CMake hiện đại, nên nghĩ mọi thứ xoay quanh `target`.

Ví dụ:

```cmake
add_executable(lesson_05 main.c)
```

tạo một target executable.

```cmake
add_library(led_driver STATIC src/led.c)
```

tạo một target static library.

Target giống như một "đơn vị build". Mỗi target có thể có:

```text
source files
include directories
compile definitions
compile options
link libraries
link options
```

## 4. add_executable làm gì?

`add_executable` tạo chương trình cuối cùng.

Trong embedded STM32, executable thường là file:

```text
lesson_05.elf
```

Ví dụ:

```cmake
add_executable(lesson_05
    main.c
    startup_stm32f103.c
    system_stm32f10x.c
)
```

Nghĩa là các file `.c` này sẽ được compile thành `.o`, rồi linker ghép lại thành `.elf`.

## 5. target_sources làm gì?

Hai cách này gần giống nhau:

```cmake
add_executable(app main.c led.c)
```

và:

```cmake
add_executable(app)

target_sources(app PRIVATE
    main.c
    led.c
)
```

`target_sources` hữu ích khi muốn tạo target trước, rồi thêm source sau.

Nó cũng giúp file CMake dễ chia module hơn.

## 6. include directory dùng để làm gì?

Include directory phục vụ bước compile.

Ví dụ code có:

```c
#include "led.h"
```

thì compiler phải biết `led.h` nằm ở đâu.

CMake khai báo bằng:

```cmake
target_include_directories(led_driver PUBLIC
    include
)
```

Nhớ kỹ:

```text
include directory chỉ dùng lúc compile .c -> .o
include directory không phải thư viện để linker link
```

Nếu thiếu include, lỗi thường là:

```text
fatal error: xxx.h: No such file or directory
```

## 7. compile definitions là gì?

Compile definition là macro truyền vào compiler.

Ví dụ:

```cmake
target_compile_definitions(stm32_spl PUBLIC
    STM32F10X_MD
    USE_STDPERIPH_DRIVER
)
```

tương đương khi compile có:

```bash
-DSTM32F10X_MD -DUSE_STDPERIPH_DRIVER
```

Trong STM32 SPL, các define này rất quan trọng vì header dùng chúng để chọn đúng dòng chip và bật cấu hình SPL.

Nếu thiếu, thường gặp lỗi kiểu:

```text
Please select first the target STM32F10x device
```

## 8. compile options là gì?

Compile options là cờ truyền cho compiler khi compile `.c` thành `.o`.

Ví dụ:

```cmake
target_compile_options(app PRIVATE
    -mcpu=cortex-m3
    -mthumb
    -ffunction-sections
    -fdata-sections
    -Wall
    -Wextra
)
```

Với Cortex-M3, rất quan trọng:

```text
-mcpu=cortex-m3
-mthumb
```

Vì Cortex-M3 chỉ chạy Thumb. Nếu `main.c` không được compile với `-mthumb`, firmware có thể build được nhưng chạy sai hoặc hardfault.

## 9. link options là gì?

Link options dùng ở bước link cuối cùng để tạo `.elf`.

Ví dụ:

```cmake
target_link_options(app PRIVATE
    -mcpu=cortex-m3
    -mthumb
    -nostartfiles
    -Wl,--gc-sections
    -Wl,-Map=app.map
    -Tstm32f103c8.ld
)
```

Các cờ này chủ yếu dành cho firmware executable, không phải static library.

Ý nghĩa cơ bản:

```text
-Txxx.ld          dùng linker script
-nostartfiles    không dùng startup mặc định của libc
-Wl,--gc-sections bỏ section không dùng
-Wl,-Map=xxx.map tạo file map để soi memory/symbol
```

Nếu thiếu linker script, firmware bare-metal thường không biết đặt code/data vào Flash/RAM thế nào.

## 10. Static library .a là gì?

Static library `.a` là gói chứa nhiều file `.o`.

Quy trình:

```text
led.c -> led.o
gpio.c -> gpio.o
rcc.c -> rcc.o
gpio.o + rcc.o -> libstm32_spl.a
led.o -> libled_driver.a
main.o + startup.o + system.o + .a -> lesson_05.elf
```

Tạo static library bằng:

```cmake
add_library(led_driver STATIC
    src/led.c
)
```

Static library không phải firmware hoàn chỉnh. Nó chưa cần linker script.

Static library thường cần:

```text
target_sources / add_library sources
target_include_directories
target_compile_definitions
target_compile_options
```

Firmware executable mới cần:

```text
target_link_libraries
target_link_options
linker script
```

## 11. target_link_libraries làm gì?

`target_link_libraries` nói rằng target này cần link với target/thư viện khác.

Ví dụ:

```cmake
target_link_libraries(lesson_05 PRIVATE
    led_driver
)
```

Nghĩa là `lesson_05.elf` cần code trong `led_driver`.

Nếu `led_driver` lại link với `stm32_spl`, CMake sẽ biết thứ tự phụ thuộc:

```text
lesson_05 -> led_driver -> stm32_spl
```

## 12. PUBLIC, PRIVATE, INTERFACE khác nhau thế nào?

Đây là câu hỏi phỏng vấn rất hay gặp.

```text
PRIVATE
chỉ target hiện tại dùng

PUBLIC
target hiện tại dùng, target nào link nó cũng được thừa hưởng

INTERFACE
target hiện tại không dùng, chỉ truyền cho target khác dùng
```

Ví dụ:

```cmake
target_include_directories(led_driver PUBLIC
    include
)
```

`led_driver` cần `include`, và `app` link `led_driver` cũng cần thấy `led.h`, nên dùng `PUBLIC`.

Ví dụ khác:

```cmake
target_link_libraries(led_driver PUBLIC
    stm32_spl
)
```

`led_driver` dùng SPL. `app` dùng `led_driver` cũng cần nhận include/define từ SPL, nên để `PUBLIC`.

Với app cuối:

```cmake
target_link_libraries(lesson_05 PRIVATE
    led_driver
)
```

Để `PRIVATE` vì `lesson_05` là firmware cuối cùng. Không có target nào khác link lại `lesson_05`.

## 13. Vì sao led_driver include để PUBLIC?

`main.c` có:

```c
#include "led.h"
```

Mà `led.h` nằm trong:

```text
led_driver/include
```

Nên `led_driver` phải khai báo:

```cmake
target_include_directories(led_driver PUBLIC
    include
)
```

Nếu để `PRIVATE`, `led_driver` tự compile được, nhưng `app/main.c` không thấy `led.h`.

## 14. Vì sao stm32_spl definitions để PUBLIC?

SPL/CMSIS cần:

```text
STM32F10X_MD
USE_STDPERIPH_DRIVER
```

Không chỉ file trong `stm32_spl` cần, mà `led_driver` và `app` cũng có thể include các header liên quan.

Vì vậy:

```cmake
target_compile_definitions(stm32_spl PUBLIC
    STM32F10X_MD
    USE_STDPERIPH_DRIVER
)
```

giúp truyền define theo chuỗi dependency.

## 15. add_subdirectory làm gì?

`add_subdirectory` bảo CMake đi vào thư mục con và đọc `CMakeLists.txt` ở đó.

Ví dụ root:

```cmake
add_subdirectory(stm32_spl)
add_subdirectory(led_driver)
add_subdirectory(app)
```

Ý nghĩa:

```text
stm32_spl/CMakeLists.txt   tạo libstm32_spl.a
led_driver/CMakeLists.txt  tạo libled_driver.a
app/CMakeLists.txt         tạo lesson_05.elf
```

Root không cần biết chi tiết từng module compile source nào.

## 16. Vì sao output nằm trong build/app?

Nếu target được khai báo trong:

```text
app/CMakeLists.txt
```

thì output mặc định nằm trong:

```text
build/app/
```

Ví dụ:

```text
build/app/lesson_05.elf
```

CMake mirror cấu trúc source vào thư mục build.

Nếu `build.sh` tìm ở:

```text
build/lesson_05.elf
```

thì sẽ báo không thấy firmware.

## 17. Startup và system nên đặt ở đâu?

Trong embedded, firmware cuối thường nên trực tiếp compile:

```text
startup_stm32f103.c
system_stm32f10x.c
```

Lý do:

```text
startup chứa vector table và Reset_Handler
system chứa SystemInit
firmware nào cũng cần đúng startup/system của board đó
```

Nếu nhét startup vào static library, linker có thể chỉ lấy object khi thấy symbol cần. Với vector table, cách này dễ gây khó hiểu khi mới học.

Cách dễ hiểu:

```cmake
add_executable(lesson_05
    main.c
    startup_stm32f103.c
    system_stm32f10x.c
)
```

## 18. Vì sao thiếu libc syscall?

Nếu firmware bare-metal bị lỗi:

```text
undefined reference to `_write'
undefined reference to `_sbrk'
undefined reference to `_read'
undefined reference to `_lseek'
undefined reference to `_exit'
```

thường là do linker đang kéo `newlib/libc` theo kiểu chương trình C thông thường.

Nguyên nhân hay gặp:

```text
thiếu -nostartfiles
thiếu linker script
link sai kiểu firmware bare-metal
dùng printf/malloc nhưng chưa có syscall
```

Với bài blink LED đơn giản, thường chưa cần libc syscall.

## 19. Linker script dùng để làm gì?

Linker script nói cho linker biết memory map của chip.

Ví dụ STM32F103:

```text
FLASH bắt đầu 0x08000000
RAM bắt đầu 0x20000000
vector table đặt đầu Flash
.text đặt trong Flash
.data load từ Flash nhưng chạy ở RAM
.bss đặt trong RAM
stack ở cuối RAM
```

Không có linker script, linker không biết đặt firmware đúng vị trí cho MCU.

## 20. Cách đọc lỗi CMake/build

Khi lỗi, nên nhìn lỗi theo thứ tự:

1. Lỗi xảy ra lúc configure hay build?
2. Nếu build, lỗi ở compile hay link?
3. File `.c` nào đang bị lỗi?
4. Target nào đang compile file đó?
5. Target đó có đủ include/define/options chưa?

Ví dụ:

```text
fatal error: stm32f10x_gpio.h: No such file or directory
```

Đây là lỗi compile.

Cần hỏi:

```text
file nào include stm32f10x_gpio.h?
file đó đang nằm trong target nào?
target đó có target_include_directories tới SPL/inc chưa?
```

Ví dụ:

```text
undefined reference to led_init
```

Đây là lỗi link.

Cần hỏi:

```text
led.c đã được compile chưa?
target chứa led.c có được link vào firmware chưa?
target_link_libraries có thiếu led_driver không?
```

## 21. Bản chất các bài đã học

`01_add_executable`

```text
học tạo target executable đơn giản
```

`02_target_sources`

```text
học thêm source vào target sau khi target đã tạo
```

`03_include_directories`

```text
học header nằm thư mục riêng thì phải khai báo include path
```

`04_static_library`

```text
học đóng code thành .a rồi link vào firmware chính
```

`05_add_subdirectory`

```text
học chia project thành nhiều thư mục, mỗi thư mục tự quản lý target của nó
```

## 22. Công thức nhớ nhanh

Nếu file `.c` include header:

```text
target đang compile file .c đó phải có include directory
```

Nếu file `.c` cần macro:

```text
target đang compile file .c đó phải có compile definition
```

Nếu target tạo object cho Cortex-M3:

```text
target đó phải compile với -mcpu=cortex-m3 -mthumb
```

Nếu target tạo firmware `.elf`:

```text
target đó phải có linker script và link options
```

Nếu thư viện có header public cho người khác include:

```text
target_include_directories(lib PUBLIC include)
```

Nếu thư viện dùng thư viện khác và người dùng nó cũng cần dependency đó:

```text
target_link_libraries(lib PUBLIC dependency)
```

Nếu app cuối dùng thư viện:

```text
target_link_libraries(app PRIVATE lib)
```

## 23. Câu trả lời phỏng vấn ngắn gọn

Hỏi: CMake có compile code không?

Trả lời:

```text
Không trực tiếp. CMake sinh hệ build như Makefile/Ninja, hệ build đó mới gọi compiler/linker.
```

Hỏi: Target là gì?

Trả lời:

```text
Target là đơn vị build trong CMake, ví dụ executable hoặc library. Mỗi target mang source, include, define, compile option, link library, link option của nó.
```

Hỏi: Include directory khác link library thế nào?

Trả lời:

```text
Include directory dùng lúc compile để tìm header. Link library dùng lúc link để tìm code/symbol đã compile.
```

Hỏi: Static library `.a` là gì?

Trả lời:

```text
.a là gói chứa nhiều object file .o. Nó chưa phải chương trình hoàn chỉnh. Linker sẽ lấy object cần thiết từ .a để tạo executable.
```

Hỏi: PUBLIC và PRIVATE khác gì?

Trả lời:

```text
PRIVATE chỉ áp dụng cho target hiện tại. PUBLIC áp dụng cho target hiện tại và truyền cho target nào link tới nó.
```

Hỏi: Vì sao firmware cần linker script?

Trả lời:

```text
Vì bare-metal MCU cần đặt vector table, code, data, bss, stack vào đúng địa chỉ Flash/RAM.
```

Hỏi: Vì sao static library không cần target_link_options?

Trả lời:

```text
Vì static library chỉ đóng gói .o thành .a, chưa link thành chương trình cuối. Link options dùng ở bước tạo .elf.
```

Hỏi: Vì sao add_subdirectory làm output nằm sâu hơn?

Trả lời:

```text
CMake tạo thư mục build tương ứng với thư mục source con. Target khai báo trong app/CMakeLists.txt thường ra build/app/.
```
