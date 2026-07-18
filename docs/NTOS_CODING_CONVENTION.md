# NTOS CODING CONVENTION

> **Project:** ntos — Educational RTOS for STM32F103 / ARM Cortex-M3  
> **Language:** C11 + GNU inline assembly  
> **Compiler:** `arm-none-eabi-gcc`  
> **Mục tiêu:** Code nhỏ, nhất quán, dễ đọc, dễ debug và thể hiện rõ bản chất RTOS.

---

# 1. Nguyên tắc chung

Thứ tự ưu tiên:

1. Đúng về mặt kiến trúc.
2. Dễ hiểu.
3. Dễ debug.
4. Nhất quán.
5. Ít abstraction không cần thiết.
6. Tối ưu sau cùng.

Quy tắc:

- Không viết code “thông minh” nếu code rõ ràng hơn có thể làm được.
- Không tối ưu trước khi có test chứng minh cần thiết.
- Không thêm API chỉ vì RTOS khác có.
- Không copy assembly nếu chưa giải thích được từng instruction.
- Không để warning compiler tồn tại.
- Không trộn nhiều trách nhiệm trong một module.
- Code và tài liệu phải đồng bộ.

---

# 2. Phạm vi module

## 2.1. Kernel

`kernel/` chứa logic độc lập với phần cứng:

- Task management.
- Scheduler policy.
- Task states.
- Delay.
- Ready structures.
- Semaphore.
- Queue.
- Assertions.

Kernel không được:

- Truy cập trực tiếp `SCB`, `NVIC`, `SysTick`.
- Đọc/ghi `PSP`, `MSP`, `CONTROL`, `PRIMASK`.
- Chứa inline assembly.
- Biết GPIO, UART hoặc board cụ thể.

## 2.2. Cortex-M3 port

`port/cortex_m3/` chứa phần phụ thuộc kiến trúc:

- SVC.
- PendSV.
- SysTick port hooks.
- PSP/MSP.
- CONTROL.
- PRIMASK.
- Exception priority.
- Request context switch.

Port không được:

- Chứa scheduler policy.
- Chứa application logic.
- Chứa GPIO/UART.
- Tự quản lý task table ngoài interface kernel.

## 2.3. BSP

`bsp/stm32f10x/` chứa hỗ trợ MCU/board:

- Clock.
- GPIO.
- UART.
- EXTI.
- Board mapping.
- Startup.
- Linker.
- System initialization.

BSP không được:

- Include kernel internal headers.
- Gọi semaphore, queue hoặc scheduler.
- Chứa task logic.
- Biết state của task.

## 2.4. Application, tests và examples

- Chỉ gọi public API.
- Không include `kernel/internal/*.h`.
- Không truy cập trực tiếp biến kernel nội bộ.
- Test nội bộ đặc biệt chỉ được phép phá vỡ quy tắc này khi ghi rõ mục đích.

---

# 3. Quy ước tên file

Tên file dùng chữ thường và dấu gạch dưới:

```text
ntos_task.c
ntos_kernel.c
ntos_port.c
test_context_switch.c
```

Không dùng:

```text
NtosTask.c
task-manager.c
Task.C
```

Tên module phải phản ánh đúng trách nhiệm.

---

# 4. Quy ước tên identifier

## 4.1. Public API

Public API dùng tiền tố:

```c
ntos_<module>_<action>()
```

Ví dụ:

```c
ntos_task_create_static()
ntos_delay()
ntos_sem_take()
ntos_queue_send()
```

BSP public API:

```c
bsp_<module>_<action>()
```

Ví dụ:

```c
bsp_gpio_config()
bsp_uart_write()
bsp_clock_init()
```

Board API:

```c
board_<action>()
```

Ví dụ:

```c
board_init()
board_led_toggle()
```

## 4.2. Hàm static nội bộ

Hàm `static` bỏ tiền tố `ntos_`, nhưng vẫn giữ tên module:

```c
task_build_initial_stack()
task_validate_create_arguments()
kernel_select_first_task()
port_validate_psp()
```

Không dùng tên quá chung:

```c
init()
setup()
process()
handle()
```

## 4.3. Kiểu dữ liệu

Kiểu public dùng hậu tố `_t`:

```c
ntos_tcb_t
ntos_tick_t
ntos_status_t
bsp_gpio_mode_t
```

Struct tag dùng tên đầy đủ:

```c
struct ntos_tcb
struct ntos_list_item
```

## 4.4. Enum values

Enum dùng chữ hoa và tiền tố module:

```c
NTOS_TASK_READY
NTOS_TASK_RUNNING
NTOS_ERR_INVALID_ARGUMENT
BSP_GPIO_MODE_OUTPUT_PUSH_PULL
```

## 4.5. Macro và hằng số

Dùng chữ hoa:

```c
NTOS_MAX_TASKS
NTOS_TASK_FRAME_WORDS
BSP_UART_DEFAULT_BAUD
```

Macro nội bộ file vẫn phải có tiền tố module.

## 4.6. Biến global

Biến global public phải tránh tối đa.

Global nội bộ module:

```c
static ntos_tcb_t *task_table[NTOS_MAX_TASKS];
static size_t task_count;
```

Global cần được assembly truy cập có tên rõ ràng:

```c
ntos_tcb_t *volatile ntos_current_tcb;
```

Không dùng tiền tố `g_` bắt buộc. Chỉ dùng nếu toàn project thống nhất.

## 4.7. Biến local

Tên mô tả ý nghĩa:

```c
current_task
next_task
stack_pointer
saved_irq_state
```

Không dùng:

```c
a
tmp
x
foo
data1
```

Ngoại lệ:

- `i`, `j` cho vòng lặp nhỏ.
- `sp`, `psp`, `msp` khi đúng ngữ cảnh kiến trúc.
- `tcb` khi phạm vi hàm ngắn và rõ.

---

# 5. Formatting

## 5.1. Indentation

Dùng 4 spaces.

Không dùng tab.

## 5.2. Braces

Dùng Allman style:

```c
if (condition)
{
    do_something();
}
else
{
    do_something_else();
}
```

Mọi `if`, `for`, `while` đều phải có braces, kể cả một dòng.

## 5.3. Độ dài dòng

Khuyến nghị tối đa 100 ký tự.

Cho phép dài hơn với:

- Macro đặc biệt.
- Prototype khó ngắt dòng.
- Assembly string cần giữ dễ đọc.

## 5.4. Khoảng trắng

```c
if (value == 0U)
for (size_t i = 0U; i < count; ++i)
result = left + right;
```

Không viết:

```c
if(value==0U)
result=left+right;
```

## 5.5. Khai báo biến

Khai báo gần nơi sử dụng.

```c
uint32_t *sp = stack + stack_words;
```

Không gom toàn bộ biến lên đầu hàm nếu chưa cần.

Ngoại lệ: code theo chuẩn compiler cũ không áp dụng cho project này.

---

# 6. Kiểu số và literal

## 6.1. Kiểu fixed-width

Dùng:

```c
uint8_t
uint16_t
uint32_t
int32_t
```

cho register, tick và dữ liệu có kích thước xác định.

Dùng `size_t` cho:

- Số phần tử.
- Kích thước buffer.
- Index mảng.

## 6.2. Suffix

Literal unsigned phải có `U`:

```c
0U
1U
1000U
0x01000000U
```

Không viết:

```c
0x01000000
```

nếu giá trị được dùng như `uint32_t`.

## 6.3. Cast con trỏ

Khi đổi pointer sang integer:

```c
(uint32_t)(uintptr_t)pointer
```

Không cast trực tiếp tùy tiện:

```c
(uint32_t)pointer
```

## 6.4. Boolean

Dùng `bool`, `true`, `false` khi biểu diễn logic.

Không dùng `uint32_t` làm boolean nếu không phải register bitfield.

---

# 7. Con trỏ và validation

Public API phải validate:

- Pointer bắt buộc khác `NULL`.
- Enum nằm trong range.
- Size hợp lệ.
- Alignment hợp lệ.
- State hợp lệ.

Ví dụ:

```c
if ((tcb == NULL) || (stack == NULL) || (entry == NULL))
{
    return NTOS_ERR_INVALID_ARGUMENT;
}
```

Không dereference pointer trước validation.

---

# 8. Error handling

## 8.1. Public API

Public API có thể thất bại phải trả `ntos_status_t`.

Không dùng `-1`, `0`, `1` mơ hồ.

## 8.2. Internal invariant

Invariant nội bộ dùng:

```c
NTOS_ASSERT(condition);
```

Ví dụ:

```c
NTOS_ASSERT(ntos_current_tcb != NULL);
```

## 8.3. Không bỏ qua return value

Không viết:

```c
ntos_task_create_static(...);
```

nếu hàm có thể fail.

Phải kiểm tra:

```c
ntos_status_t status = ntos_task_create_static(...);

if (status != NTOS_OK)
{
    ...
}
```

---

# 9. Header files

## 9.1. Include guard

Dùng:

```c
#ifndef NTOS_TASK_H
#define NTOS_TASK_H

...

#endif
```

## 9.2. Public header

Public header:

- Chỉ khai báo API công khai.
- Không include internal header.
- Không expose symbol không cần thiết.
- Có documentation cho public API.

## 9.3. Internal header

Internal header:

- Chỉ dùng trong kernel/port.
- Không được application include.
- Chứa shared internals thật sự cần thiết.

## 9.4. Include order

Thứ tự:

```c
#include <ntos/ntos_task.h>

#include "ntos_kernel_internal.h"

#include <stddef.h>
#include <stdint.h>
```

Quy ước ưu tiên:

1. Header tương ứng của file `.c`.
2. Public project headers.
3. Internal project headers.
4. CMSIS/device headers.
5. Standard library headers.

Không include thừa.

---

# 10. Documentation

## 10.1. File header

Mỗi file chính có:

```c
/**
 * @file ntos_task.c
 * @brief Tạo task tĩnh và quản lý initial context.
 */
```

## 10.2. Public function

Public API phải có:

- Mô tả.
- Tham số.
- Return value.
- Context hợp lệ.
- Có block hay không.
- Có gọi từ ISR hay không.

Ví dụ:

```c
/**
 * @brief Nhường CPU cho task khác.
 * @note Chỉ gọi từ task context.
 */
void ntos_yield(void);
```

## 10.3. Comment

Comment giải thích **tại sao**, không lặp lại code.

Tốt:

```c
/* stack_pointer phải ở offset 0 để PendSV lưu PSP trực tiếp vào TCB. */
```

Không tốt:

```c
/* Gán sp vào stack_pointer. */
tcb->stack_pointer = sp;
```

---

# 11. Function design

## 11.1. Một hàm, một trách nhiệm

Hàm dài phải được tách khi có nhiều bước logic khác nhau.

Ví dụ:

```c
task_validate_create_arguments()
task_fill_stack()
task_build_initial_stack()
```

Không tách quá mức thành helper một dòng nếu không tăng độ rõ.

## 11.2. Static helper

Helper chỉ dùng trong một file phải là `static`.

## 11.3. Noreturn

Hàm không return phải khai báo:

```c
void ntos_start(void) __attribute__((noreturn));
```

hoặc:

```c
_Noreturn void ntos_start(void);
```

Project chọn một cách và dùng nhất quán.

## 11.4. Không dùng recursion trong kernel

Kernel và port không dùng recursion.

---

# 12. Assembly convention

## 12.1. Inline assembly

Chỉ dùng trong `port/`.

Không dùng inline assembly trong kernel hoặc BSP trừ lý do đặc biệt được tài liệu hóa.

## 12.2. Naked handler

Naked function chỉ chứa assembly:

```c
__attribute__((naked)) void PendSV_Handler(void)
{
    __asm volatile(
        "...\n"
    );
}
```

Không được có:

```c
uint32_t value;
if (...)
helper();
```

trong naked function.

## 12.3. Comment assembly

Mỗi nhóm instruction phải nói rõ:

- Input.
- Output.
- Register clobber.
- Stack đang dùng.
- Mục đích.

Ví dụ:

```c
"mrs     r0, psp\n"           /* r0 = task PSP sau hardware stacking */
"stmdb   r0!, {r4-r11}\n"     /* save software context, r0 = saved PSP */
```

## 12.4. Gọi C từ assembly

Trước `bl` phải bảo đảm:

- LR chứa EXC_RETURN được lưu.
- MSP align 8 byte.
- Caller-saved register được coi là clobbered.
- Callee tuân thủ AAPCS.

Ví dụ:

```asm
push    {r3, lr}
bl      ntos_scheduler_select_next
pop     {r3, lr}
```

## 12.5. Không hard-code offset tùy tiện

Chỉ hard-code offset khi có compile-time assert:

```c
_Static_assert(offsetof(ntos_tcb_t, stack_pointer) == 0U,
               "stack_pointer must be first");
```

---

# 13. Interrupt convention

- ISR phải ngắn.
- ISR không block.
- ISR không busy-wait dài.
- ISR không gọi API task-context.
- ISR không dùng heap.
- ISR chỉ xử lý phần tối thiểu và đánh thức task nếu cần.
- Context switch chỉ thực hiện trong PendSV.

Tên handler theo vector table:

```c
void SVC_Handler(void);
void PendSV_Handler(void);
void SysTick_Handler(void);
```

---

# 14. Critical section

Critical section chỉ bảo vệ dữ liệu kernel chia sẻ:

- Task table.
- Current task metadata.
- Ready lists.
- Wait lists.
- Queue indexes/count.
- Semaphore count.

Không đặt trong critical section:

- UART print dài.
- Delay loop.
- Callback application.
- Copy buffer lớn nếu tránh được.

Mọi API disable interrupt phải restore đúng trạng thái cũ, không enable mù.

---

# 15. Task state convention

Mọi thay đổi state phải trả lời được:

1. Task đang ở state nào?
2. Task đang nằm trong cấu trúc nào?
3. Sự kiện nào yêu cầu PendSV?

Ví dụ:

```text
RUNNING → READY
current task vẫn trong task table
ntos_yield() pend PendSV
```

Không đổi state mà không cập nhật membership tương ứng.

---

# 16. CMake convention

Mỗi module có target riêng:

```text
ntos_kernel
ntos_port
ntos_bsp
```

Dùng target-based CMake:

```cmake
target_sources()
target_include_directories()
target_link_libraries()
```

Không dùng global:

```cmake
include_directories()
add_definitions()
```

trừ khi có lý do rõ.

`PUBLIC`, `PRIVATE`, `INTERFACE` phải phản ánh dependency thật.

Build directory không commit vào Git.

---

# 17. Test convention

Mỗi feature phải có ít nhất:

- Happy path.
- Invalid argument.
- Boundary case.
- Invariant check.
- Target test nếu liên quan CPU context.

Không dùng LED làm test duy nhất.

Dùng:

- `volatile` debug variables.
- Counters.
- GDB register inspection.
- Stack range check.
- Long-run test.

Tên test:

```text
test_initial_stack.c
test_first_task.c
test_context_switch.c
test_register_preservation.c
```

---

# 18. Git convention

Commit phải nhỏ và có một mục đích.

Format:

```text
type: description
```

Loại commit:

```text
feat:
fix:
refactor:
docs:
test:
build:
chore:
```

Ví dụ:

```text
feat: start first task using SVC and PSP
test: add initial stack frame validation
build: expose NTOS public include directory to port
docs: document cooperative context switch flow
```

Không commit:

- Build artifacts.
- File editor tạm.
- Thay đổi không liên quan chung một commit.

---

# 19. Quy tắc review trước khi merge

Mỗi thay đổi phải trả lời:

```text
1. Module nào chịu trách nhiệm?
2. API public nào thay đổi?
3. Task state nào thay đổi?
4. Dữ liệu nào cần critical section?
5. Có gọi từ ISR không?
6. Có block không?
7. PendSV có được yêu cầu không?
8. Invariant nào được giữ?
9. Test nào chứng minh?
10. Có thêm abstraction không cần thiết không?
```

---

# 20. Ví dụ code chuẩn

```c
ntos_status_t ntos_task_create_static(
    ntos_tcb_t *tcb,
    const char *name,
    ntos_task_entry_t entry,
    void *argument,
    uint32_t *stack,
    size_t stack_words,
    ntos_priority_t priority)
{
    if ((tcb == NULL) || (stack == NULL) || (entry == NULL))
    {
        return NTOS_ERR_INVALID_ARGUMENT;
    }

    if (priority >= NTOS_MAX_PRIORITIES)
    {
        return NTOS_ERR_INVALID_PRIORITY;
    }

    uint32_t *sp = task_build_initial_stack(
        stack,
        stack_words,
        entry,
        argument);

    tcb->stack_pointer = sp;
    tcb->stack_low = stack;
    tcb->stack_high = stack + stack_words;
    tcb->priority = priority;
    tcb->state = NTOS_TASK_READY;

    return NTOS_OK;
}
```

---

# 21. Các điều cấm

Không được:

```text
- Include kernel internal header từ application.
- Gọi scheduler từ BSP.
- Viết C statement trong naked handler.
- Dùng magic number không comment.
- Bỏ qua compiler warning.
- Dùng malloc trong kernel core.
- Save/restore context ngoài PendSV, trừ start first task bằng SVC.
- Dùng busy-wait thay cho blocking khi feature blocking đã tồn tại.
- Thêm feature ngoài roadmap hiện tại.
- Refactor lớn khi chưa có test bảo vệ.
```

---

# 22. Quy tắc cuối cùng

> Code NTOS phải cho phép người đọc trả lời được:
>
> 1. CPU context đang nằm ở đâu?
> 2. Task hiện tại là task nào?
> 3. Vì sao scheduler chọn task tiếp theo?
> 4. Task đang READY, RUNNING hay BLOCKED?
> 5. Module nào sở hữu dữ liệu đang thay đổi?
>
> Nếu code làm các câu trả lời này khó hơn, code cần được đơn giản hóa.
