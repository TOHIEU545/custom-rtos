# NTOS DEVELOPMENT ROADMAP

> **Tên project:** ntos  
> **Ý nghĩa tên:** Nghĩa Trụ Operating System  
> **Mục tiêu:** Tự xây dựng một RTOS nhỏ trên STM32F103/Cortex-M3 để hiểu bản chất task, context switch, scheduler, blocking, synchronization và IPC.  
> **Định hướng:** Educational RTOS — ưu tiên dễ hiểu, dễ kiểm chứng, ít tính năng, không hướng tới sản phẩm thương mại.  
> **Tài liệu đồng hành:** `ntos_design.md`

---

# 1. Cách sử dụng roadmap

Roadmap này là tài liệu làm việc xuyên suốt project. Không nên đọc một lần rồi để đó. Trước mỗi chặng:

```text
[ ] Đọc mục tiêu của chặng.
[ ] Trả lời các câu hỏi “tại sao”.
[ ] Vẽ flow hoặc state transition trước khi code.
[ ] Ghi rõ API cần có.
[ ] Ghi invariant phải giữ.
[ ] Viết test tối thiểu trước hoặc song song với code.
[ ] Chỉ đọc source tham khảo sau khi đã tự nghĩ cách làm.
```

Khi hoàn thành chặng:

```text
[ ] Demo chạy đúng.
[ ] Test biên chạy đúng.
[ ] Không còn warning compiler.
[ ] Không có HardFault ngẫu nhiên.
[ ] Có thể giải thích cơ chế mà không nhìn code.
[ ] Cập nhật tài liệu thiết kế nếu kiến trúc thay đổi.
[ ] Commit và gắn tag.
```

## 1.1. Nguyên tắc chính

1. **Không tối ưu trước khi đúng.**
2. **Không thêm tính năng vì thấy RTOS khác có.**
3. **Không copy assembly nếu chưa hiểu từng lệnh.**
4. **Không dùng LED làm test duy nhất.**
5. **Mọi task state phải tương ứng với vị trí rõ ràng trong scheduler.**
6. **Mọi context switch phải đi qua PendSV.**
7. **Mọi task phải có stack riêng.**
8. **Kernel chính phải chạy được không cần heap.**
9. **Chỉ làm những tính năng giúp hiểu bản chất.**
10. **Nếu một tính năng làm project phức tạp mà không thêm nhiều giá trị học tập, để vào phần tùy chọn.**

---

# 2. Phạm vi chính thức của ntos

## 2.1. Tính năng bắt buộc

```text
[x] Static task creation
[x] Task Control Block
[x] Stack riêng cho mỗi task
[x] Task chạy bằng PSP
[x] Interrupt chạy bằng MSP
[x] Khởi chạy task đầu bằng SVC
[x] Context switch bằng PendSV
[x] Cooperative yield
[x] Preemptive scheduling bằng SysTick
[x] Fixed-priority scheduling
[x] Round-robin giữa task cùng priority
[x] Task states
[x] Idle task
[x] Task delay
[x] Ready list
[x] Binary/counting semaphore cơ bản
[x] Semaphore give từ ISR
[x] Queue ring buffer
[x] Queue send từ ISR
[x] Assert cơ bản
[x] Stack fill pattern và kiểm tra cơ bản
[x] Demo tổng hợp
```

## 2.2. Tính năng tùy chọn

Chỉ làm nếu phần lõi đã hoàn chỉnh:

```text
[ ] Priority bitmap
[ ] Ordered delayed list
[ ] Dual delayed lists xử lý tick overflow theo kiểu FreeRTOS
[ ] Mutex đơn giản
[ ] Priority inheritance một tầng
[ ] Software timer đơn giản
[ ] WFI trong idle task
[ ] Runtime task statistics
[ ] Fixed-block memory pool
```

## 2.3. Không làm trong phiên bản học tập

```text
[ ] Dynamic task deletion
[ ] General-purpose heap bắt buộc
[ ] Recursive mutex
[ ] Multi-level transitive priority inheritance
[ ] Tickless idle hoàn chỉnh
[ ] MPU
[ ] User mode
[ ] SMP/multicore
[ ] FPU context
[ ] POSIX compatibility
[ ] Safety certification
[ ] Device framework
[ ] Networking/filesystem
[ ] FreeRTOS-compatible API
```

---

# 3. Nền tảng kỹ thuật

```text
MCU                  STM32F103
Core                 ARM Cortex-M3
ISA                  ARMv7-M
Compiler             arm-none-eabi-gcc
Language             C11 + GNU Assembly
Build                CMake hoặc Makefile
Debug                ST-Link + GDB
Kernel tick          1 kHz
Task allocation      Static
Priorities           8 mức ban đầu
Context switch       PendSV
First task start     SVC
Task stack           PSP
Handler stack        MSP
Interrupt mask       PRIMASK giai đoạn đầu
```

Quy ước priority:

```text
0 = thấp nhất
NTOS_MAX_PRIORITIES - 1 = cao nhất
```

---

# 4. Các source nên tham khảo

Không dùng source tham khảo để copy nguyên khối. Mục đích là nhìn cách người khác giải quyết cùng một vấn đề sau khi đã tự thiết kế.

## 4.1. AKOS

Nên đọc sớm vì source nhỏ và gần với mục tiêu ntos:

```text
port Cortex-M3
thread/task
priority
list
message
timer
```

Dùng để hiểu:

- TCB.
- PendSV/SVC.
- Ready list.
- Delay.
- Message queue.
- Timer task.

## 4.2. FreeRTOS

Chỉ đọc từng phần đúng thời điểm:

```text
portable/GCC/ARM_CM3/port.c
portable/GCC/ARM_CM3/portmacro.h
list.c
list.h
tasks.c
queue.c
```

Không đọc toàn bộ `tasks.c` từ đầu đến cuối.

## 4.3. RT-Thread Nano

Đọc khi đã có scheduler priority:

- Thread object.
- Scheduler.
- Timer.
- IPC.
- Cortex-M port.

## 4.4. Quy tắc đọc source

```text
1. Tự trả lời câu hỏi.
2. Tự vẽ thiết kế.
3. Tự code bản đơn giản.
4. Test.
5. Sau đó mới đọc AKOS/FreeRTOS.
6. Chỉ sửa thiết kế nếu hiểu lý do.
```

---

# 5. Bảng milestone

| Milestone | Nội dung | Kết quả chính |
|---|---|---|
| M0 | Bare-metal foundation | Build, flash, debug ổn định |
| M1 | Cortex-M exception model | Hiểu MSP/PSP/SVC/PendSV |
| M2 | Task và stack | Tạo initial context |
| M3 | First task | Một task chạy bằng PSP |
| M4 | Cooperative scheduler | Nhiều task tự yield |
| M5 | Preemptive scheduler | SysTick tự chia CPU |
| M6 | State, priority, idle | Scheduler có priority |
| M7 | Delay | Task block không busy-wait |
| M8 | Ready list | Scheduler dùng cấu trúc list |
| M9 | Semaphore và queue | Đồng bộ + IPC |
| M10 | Tích hợp và tổng kết | ntos educational release |

---

# CHẶNG 0 — BARE-METAL FOUNDATION

## 0.1. Mục tiêu

Xây một project STM32F103 tối thiểu có thể:

- Build từ command line.
- Flash bằng ST-Link.
- Debug bằng GDB.
- Chạy UART polling.
- Toggle GPIO.
- Bắt HardFault cơ bản.

## 0.2. Tại sao cần chặng này?

RTOS phụ thuộc trực tiếp vào:

- Vector table.
- Startup.
- Linker script.
- Clock.
- Interrupt.
- Stack.
- Debugger.

Nếu nền bare-metal không ổn, lỗi clock hoặc startup rất dễ bị nhầm thành lỗi context switch.

## 0.3. Câu hỏi bắt buộc

1. CPU lấy MSP ban đầu từ địa chỉ nào?
2. Địa chỉ `Reset_Handler` được lấy từ đâu?
3. Vector table gồm những gì?
4. `.data` khác `.bss` thế nào?
5. Ai copy `.data` từ Flash sang RAM?
6. Ai clear `.bss`?
7. Linker script quyết định vùng Flash/RAM thế nào?
8. Main stack được đặt ở đâu?
9. Vì sao `SystemCoreClock` phải chính xác?
10. HardFault handler nằm ở vector nào?
11. Tại sao không nên dùng HAL quá nhiều ở giai đoạn này?
12. Vì sao UART polling phù hợp để debug ban đầu?

## 0.4. File cần có

```text
bsp/stm32f103/startup_stm32f103.S
bsp/stm32f103/linker.ld
bsp/stm32f103/system_stm32f103.c
bsp/stm32f103/gpio.c
bsp/stm32f103/uart.c
main.c
```

## 0.5. Yêu cầu cần đạt

```text
[ ] Clock đúng.
[ ] LED blink.
[ ] UART in đúng baud.
[ ] Breakpoint tại Reset_Handler.
[ ] Breakpoint tại main.
[ ] Xem được MSP.
[ ] Cố tình HardFault và vào đúng handler.
[ ] Build không warning.
```

## 0.6. Test

- Reset 50 lần.
- Power cycle nhiều lần.
- Flash lại firmware.
- Kiểm tra biến `.data`.
- Kiểm tra biến `.bss`.
- In địa chỉ Flash/RAM qua map file.

## 0.7. Điều kiện qua chặng

Bạn phải giải thích được toàn bộ flow:

```text
Reset
→ lấy MSP
→ lấy Reset_Handler
→ copy .data
→ clear .bss
→ system init
→ main
```

## 0.8. Không được đi tiếp nếu

- Chỉ build được trong IDE.
- Không hiểu linker script.
- Clock chưa xác minh.
- Không biết MSP hiện tại nằm ở đâu.

## 0.9. Tag

```text
v0.0.1-bare-metal
```

---

# CHẶNG 1 — HỌC KỸ MSP, PSP VÀ EXCEPTION MODEL

## 1.1. Mục tiêu

Hiểu chính xác Cortex-M3 vào/ra exception như thế nào, từ đó hiểu context switch.

## 1.2. Nội dung phải nắm

- Thread mode.
- Handler mode.
- MSP.
- PSP.
- CONTROL register.
- xPSR.
- Exception entry.
- Exception return.
- Hardware stack frame.
- EXC_RETURN.
- SVC.
- PendSV.
- SysTick.
- PRIMASK.
- NVIC priority.

## 1.3. Câu hỏi bắt buộc

1. `main()` ban đầu dùng MSP hay PSP?
2. Task của RTOS nên dùng stack nào?
3. Handler mode dùng stack nào?
4. CPU tự push register nào khi vào exception?
5. Vì sao R4–R11 không được tự push?
6. xPSR chứa Thumb bit ở đâu?
7. EXC_RETURN là gì?
8. `0xFFFFFFFD` có ý nghĩa gì?
9. Làm sao biết exception frame nằm trên MSP hay PSP?
10. CONTROL.SPSEL dùng để làm gì?
11. Tại sao cần ISB sau khi thay CONTROL?
12. SVC dùng làm gì?
13. PendSV dùng làm gì?
14. Vì sao PendSV phải có priority thấp nhất?
15. SysTick nên làm gì và không nên làm gì?
16. Tail chaining là gì?
17. Điều gì xảy ra nếu PendSV pending trong lúc ISR khác đang chạy?
18. PRIMASK chặn gì?
19. NMI/HardFault có bị PRIMASK chặn không?
20. Vì sao handler stack và task stack nên tách riêng?

## 1.4. Bài thực hành

### Bài A — SysTick

- SysTick 1 ms.
- Tăng `volatile uint32_t tick`.
- Toggle LED mỗi 1000 tick.

### Bài B — PendSV thủ công

- Set `SCB->ICSR` để pend PendSV.
- Breakpoint trong `PendSV_Handler`.
- Quan sát IPSR.
- Quan sát LR.
- Xác định EXC_RETURN.

### Bài C — SVC

- Gọi `SVC #0`.
- Đọc stack frame.
- Xác định stack frame nằm trên MSP hay PSP.
- Đọc immediate của instruction SVC.

### Bài D — PSP

- Tạo một array làm stack thử.
- Set PSP.
- Đọc lại PSP.
- Chưa chạy task.

## 1.5. Tài liệu phải tự viết

Trong `docs/context-switch.md`:

```text
- Sơ đồ MSP/PSP.
- Hardware frame.
- EXC_RETURN.
- Flow SVC.
- Flow PendSV.
```

## 1.6. Test cần đạt

```text
[ ] Quan sát được hardware frame.
[ ] Biết word nào là PC.
[ ] Biết word nào là xPSR.
[ ] Xác nhận handler dùng MSP.
[ ] Xác nhận task sau này sẽ dùng PSP.
[ ] PendSV có priority thấp nhất.
```

## 1.7. Điều kiện qua chặng

Không nhìn tài liệu vẫn vẽ đúng:

```text
R0 R1 R2 R3 R12 LR PC xPSR
```

và giải thích vì sao kernel chỉ cần tự lưu:

```text
R4-R11
```

## 1.8. Không được đi tiếp nếu

- Chưa hiểu EXC_RETURN.
- Chưa phân biệt Thread/Handler.
- Chưa phân biệt MSP/PSP.
- Chỉ biết “PendSV để switch” nhưng không biết tại sao.

## 1.9. Source tham khảo

- AKOS Cortex-M3 port.
- FreeRTOS ARM_CM3 port, chỉ xem sau khi tự làm bài thực hành.

## 1.10. Tag

```text
v0.0.2-core-exceptions
```

---

# CHẶNG 2 — TCB VÀ INITIAL TASK STACK

## 2.1. Mục tiêu

Tạo một task object hoàn chỉnh và dựng initial context trên stack.

## 2.2. Tại sao cần initial context?

Một task mới chưa từng chạy. Nhưng PendSV restore phải coi nó giống một task từng bị interrupt. Vì vậy cần tạo sẵn một frame giả.

## 2.3. TCB tối thiểu

```c
typedef void (*ntos_task_entry_t)(void *argument);

typedef enum
{
    NTOS_TASK_UNUSED = 0,
    NTOS_TASK_READY,
    NTOS_TASK_RUNNING,
    NTOS_TASK_BLOCKED
} ntos_task_state_t;

typedef struct
{
    uint32_t *stack_pointer;

    uint32_t *stack_low;
    uint32_t *stack_high;
    size_t stack_words;

    ntos_task_entry_t entry;
    void *argument;

    const char *name;
    uint8_t priority;
    ntos_task_state_t state;
} ntos_tcb_t;
```

`stack_pointer` nên là field đầu tiên.

## 2.4. Initial stack frame

```text
High address

xPSR = 0x01000000
PC   = task entry
LR   = ntos_task_return_error
R12
R3
R2
R1
R0   = argument
R11
R10
R9
R8
R7
R6
R5
R4

Low address
```

## 2.5. Câu hỏi bắt buộc

1. Stack Cortex-M tăng hay giảm?
2. Vì sao stack phải align 8 byte?
3. Vì sao R0 chứa task argument?
4. PC phải có trạng thái Thumb thế nào?
5. xPSR ban đầu cần giá trị nào?
6. LR của task nên trỏ tới đâu?
7. Nếu task return thì làm gì?
8. Vì sao điền pattern vào register?
9. Vì sao fill stack bằng `0xA5`?
10. `stack_low` và `stack_high` định nghĩa thế nào?
11. Stack size dùng byte hay word?
12. Vì sao không malloc stack?

## 2.6. API

```c
void ntos_init(void);

ntos_status_t ntos_task_create_static(
    ntos_tcb_t *tcb,
    const char *name,
    ntos_task_entry_t entry,
    void *argument,
    uint32_t *stack,
    size_t stack_words,
    uint8_t priority);
```

## 2.7. Invariant

- Stack pointer nằm trong vùng stack.
- Stack aligned.
- Entry không NULL.
- Priority hợp lệ.
- TCB chưa được đăng ký trước đó.
- LR không trỏ vào địa chỉ rác.
- xPSR có Thumb bit.
- Argument nằm đúng vị trí R0.

## 2.8. Test

- Entry hợp lệ.
- Entry NULL.
- Stack nhỏ.
- Stack không aligned.
- Argument NULL.
- Argument là struct pointer.
- Priority invalid.
- Dump frame ra UART hoặc debugger.

## 2.9. Kết quả cần đạt

Bạn chỉ vào từng word trong stack và giải thích được register tương ứng.

## 2.10. Điều kiện qua chặng

Initial frame đúng và được kiểm chứng bằng debugger.

## 2.11. Source tham khảo

- AKOS stack initialization.
- FreeRTOS `pxPortInitialiseStack()`.

## 2.12. Tag

```text
v0.1.0-task-stack
```

---

# CHẶNG 3 — START FIRST TASK BẰNG SVC

## 3.1. Mục tiêu

Chạy task đầu tiên bằng PSP, trong khi interrupt vẫn dùng MSP.

## 3.2. Câu hỏi bắt buộc

1. Vì sao không gọi thẳng task entry từ `main()`?
2. Nếu gọi thẳng thì task dùng stack nào?
3. Vì sao cần vào Handler mode trước?
4. SVC handler restore R4–R11 hay hardware frame?
5. CPU restore hardware frame khi nào?
6. CONTROL.SPSEL được set lúc nào?
7. Tại sao scheduler start không return?
8. SysTick nên enable trước hay sau first task?
9. Interrupt enable tại thời điểm nào?
10. Task return error hoạt động ra sao?

## 3.3. Flow

```text
main / MSP
  |
ntos_start()
  |
select first TCB
  |
SVC
  |
restore R4-R11
  |
set PSP
  |
set CONTROL.SPSEL
  |
exception return
  |
task / PSP
```

## 3.4. API

```c
void ntos_start(void) __attribute__((noreturn));
```

## 3.5. Yêu cầu code

- Global `ntos_current_tcb`.
- Chọn first task.
- Set PendSV priority.
- Set SysTick priority.
- SVC handler.
- Start first task.
- Task return trap.

## 3.6. Test

```text
[ ] Task chạy bằng PSP.
[ ] Handler dùng MSP.
[ ] Local variable nằm trong task stack.
[ ] Task argument đúng.
[ ] Task return bị assert.
[ ] SysTick vẫn hoạt động.
```

## 3.7. Điều kiện qua chặng

Một task chạy ổn ít nhất 30 phút bằng PSP.

## 3.8. Không được đi tiếp nếu

- Task vẫn dùng MSP.
- Không biết EXC_RETURN đang dùng.
- Task return gây HardFault khó hiểu.
- Stack guard bị hỏng.

## 3.9. Tag

```text
v0.1.1-first-task
```

---

# CHẶNG 4 — COOPERATIVE SCHEDULER BẰNG PENDSV

## 4.1. Mục tiêu

Nhiều task tự gọi `ntos_yield()` để nhường CPU.

## 4.2. Scheduler tạm thời

Dùng array:

```c
static ntos_tcb_t *task_table[NTOS_MAX_TASKS];
static size_t task_count;
static size_t current_task_index;
```

Chưa cần linked list.

## 4.3. Câu hỏi bắt buộc

1. Vì sao `yield()` chỉ pend PendSV?
2. Vì sao không save register ngay trong `yield()`?
3. CPU tự save register lúc nào?
4. R4–R11 được save ra sao?
5. PSP được lưu vào đâu?
6. Task kế tiếp được chọn ở đâu?
7. Tại sao LR/EXC_RETURN phải được giữ khi gọi hàm C?
8. AAPCS quy định callee-saved register nào?
9. Vì sao PendSV là nơi duy nhất thay current TCB?
10. Nếu chỉ có một task thì yield làm gì?

## 4.4. PendSV flow

```text
Task A calls ntos_yield
→ PendSV pending
→ exception entry pushes hardware frame
→ save R4-R11
→ save PSP into Task A TCB
→ scheduler selects Task B
→ load Task B PSP
→ restore R4-R11
→ exception return
→ Task B resumes
```

## 4.5. API

```c
void ntos_yield(void);
```

## 4.6. Invariant

- Chỉ PendSV thay `ntos_current_tcb`.
- Saved PSP hợp lệ.
- R4–R11 được bảo toàn.
- Task table không có duplicate.
- Không chọn task UNUSED.
- Task state hợp lệ.

## 4.7. Test bắt buộc

### Local variable test

Mỗi task giữ local magic value.

### Register test

R4–R11 chứa pattern riêng.

### Stress

```text
1.000.000 context switches
0 HardFault
0 assert
0 corruption
```

### Stack isolation

Mỗi stack có pattern riêng.

## 4.8. Điều kiện qua chặng

Context switch core vượt stress test.

## 4.9. Không được đi tiếp nếu

- Chỉ nhìn LED.
- Chưa test R4–R11.
- Không hiểu từng instruction trong PendSV.
- HardFault ngẫu nhiên vẫn còn.

## 4.10. Source tham khảo

- AKOS PendSV.
- FreeRTOS ARM_CM3 PendSV.
- RT-Thread Cortex-M switch.

## 4.11. Tag

```text
v0.2.0-cooperative
```

---

# CHẶNG 5 — PREEMPTIVE ROUND-ROBIN BẰNG SYSTICK

## 5.1. Mục tiêu

Task bị preempt dù không gọi yield.

## 5.2. Câu hỏi bắt buộc

1. Vì sao SysTick không trực tiếp đổi PSP?
2. SysTick làm gì?
3. PendSV làm gì?
4. Nếu SysTick xảy ra khi ISR khác đang chạy thì sao?
5. Nếu nhiều lần pend PendSV thì sao?
6. Time slice là gì?
7. Tick 1 ms có ý nghĩa gì?
8. Tick có phải thời gian tuyệt đối chính xác không?
9. Nếu ISR kéo dài quá lâu thì scheduler bị ảnh hưởng ra sao?
10. Khi scheduler chưa start, SysTick phải xử lý thế nào?

## 5.3. Implementation cơ bản

```c
void SysTick_Handler(void)
{
    ntos_tick_count++;

    if (ntos_scheduler_running)
    {
        ntos_port_request_context_switch();
    }
}
```

## 5.4. Demo

Hai task CPU-bound không gọi yield.

## 5.5. Test

- 2 task.
- 3 task.
- 8 task.
- Tick 100 Hz.
- Tick 1 kHz.
- Task có yield và không yield.
- Interrupt timer xen vào.
- GPIO trace SysTick/PendSV.

## 5.6. Kết quả cần đạt

Mọi task cùng mức đều có cơ hội chạy.

## 5.7. Điều kiện qua chặng

Không task CPU-bound nào chiếm toàn bộ CPU khi có task cùng priority.

## 5.8. Tag

```text
v0.2.1-preemptive
```

---

# CHẶNG 6 — TASK STATES, PRIORITY VÀ IDLE TASK

## 6.1. Mục tiêu

Scheduler có fixed priority và state machine rõ ràng.

## 6.2. States

```c
typedef enum
{
    NTOS_TASK_READY,
    NTOS_TASK_RUNNING,
    NTOS_TASK_BLOCKED
} ntos_task_state_t;
```

SUSPENDED có thể bỏ khỏi phiên bản đầu.

## 6.3. Idle task

```c
static void ntos_idle_task(void *argument)
{
    (void)argument;

    for (;;)
    {
        /* Có thể thêm WFI sau. */
    }
}
```

## 6.4. Câu hỏi bắt buộc

1. READY khác RUNNING thế nào?
2. BLOCKED khác READY thế nào?
3. Current task có nằm trong ready structure không?
4. Idle task cần vì sao?
5. Idle task priority bao nhiêu?
6. Idle task có block không?
7. High-priority task luôn READY thì Low có chạy không?
8. Đây có phải bug không?
9. Yield của High có cho Low chạy không?
10. Round-robin chỉ áp dụng giữa task nào?
11. Task mới READY có preempt current không?
12. `base_priority` và `effective_priority` có cần ngay không?

## 6.5. Scheduler O(n)

Trước khi làm ready list, quét task table để chọn highest READY.

## 6.6. Rules

```text
- Highest READY priority luôn được chọn.
- Task cao hơn vừa READY sẽ preempt task thấp hơn.
- Task thấp hơn READY không preempt task cao.
- Task cùng priority round-robin.
- Yield chỉ nhường cho task cùng priority hoặc cao hơn.
- Idle chỉ chạy khi không task application nào READY.
```

## 6.7. Test

- High periodic + Low CPU-bound.
- High luôn READY.
- Ba task cùng priority.
- Task cùng priority yield.
- Task thấp wake.
- Task cao wake.
- Idle only.
- Priority invalid.

## 6.8. Điều kiện qua chặng

Bạn dự đoán đúng task nào chạy trong mọi tình huống test.

## 6.9. Tag

```text
v0.3.0-priority-states
```

---

# CHẶNG 7 — TASK DELAY VÀ BLOCKING

## 7.1. Mục tiêu

Task ngủ theo tick mà không busy-wait.

## 7.2. Phiên bản đầu

Dùng:

```c
uint32_t wake_tick;
```

trong TCB và quét task table mỗi tick.

## 7.3. API

```c
void ntos_delay(uint32_t ticks);
```

Tùy chọn:

```c
void ntos_delay_until(
    uint32_t *last_wake_tick,
    uint32_t period_ticks);
```

## 7.4. Câu hỏi bắt buộc

1. Task gọi delay thì rời state nào?
2. Nó chuyển vào state nào?
3. Ai đánh thức task?
4. Wake task có preempt current không?
5. Delay 0 nghĩa là gì?
6. Delay 1 tick có sai số thế nào?
7. Tick overflow là gì?
8. Vì sao `now >= wake` có vấn đề?
9. Signed subtraction dùng được trong giới hạn nào?
10. Delay-until khác delay thế nào?
11. Vì sao delay-until ít drift?
12. Task BLOCKED có được scheduler chọn không?

## 7.5. Tick helper

```c
static bool ntos_tick_reached(uint32_t now, uint32_t target)
{
    return (int32_t)(now - target) >= 0;
}
```

Giới hạn delay phải được ghi rõ.

## 7.6. Invariant

- BLOCKED task không chạy.
- Wake đúng một lần.
- Task wake được chuyển READY.
- High task wake có thể preempt current.
- Delay không busy-wait.
- Idle chạy khi tất cả task block.

## 7.7. Test

- Delay 0.
- Delay 1.
- Delay 10.
- Delay 1000.
- Nhiều task cùng wake.
- High wake khi Low chạy.
- Tick gần `UINT32_MAX`.
- Delay qua overflow.
- Delay-until nếu có.

## 7.8. Demo

```text
Task A: 100 ms
Task B: 250 ms
Task C: 1000 ms
```

## 7.9. Điều kiện qua chặng

Task delay không chiếm CPU và test overflow chạy đúng.

## 7.10. Tag

```text
v0.3.1-delay
```

---

# CHẶNG 8 — READY LIST VÀ ROUND-ROBIN STRUCTURE

## 8.1. Mục tiêu

Thay scheduler quét toàn bộ task bằng ready lists dễ hiểu.

## 8.2. Phạm vi

Chỉ cần các thao tác:

```text
- add task cuối ready list
- remove task
- lấy task tiếp theo round-robin
- kiểm tra list empty
```

Không cần thư viện list quá tổng quát.

## 8.3. Cấu trúc đề xuất

```c
typedef struct ntos_list_item
{
    struct ntos_list_item *next;
    struct ntos_list_item *previous;
    void *owner;
    void *container;
} ntos_list_item_t;

typedef struct
{
    ntos_list_item_t end;
    ntos_list_item_t *index;
    uint32_t length;
} ntos_list_t;
```

TCB:

```c
ntos_list_item_t ready_item;
```

## 8.4. Ready lists

```c
static ntos_list_t ready_lists[NTOS_MAX_PRIORITIES];
```

Priority bitmap là tùy chọn.

## 8.5. Câu hỏi bắt buộc

1. Vì sao một list cho mỗi priority?
2. Vì sao task cùng priority dùng một list?
3. Round-robin cursor nằm ở đâu?
4. Intrusive list là gì?
5. Vì sao item cần owner?
6. Vì sao item cần container?
7. Sentinel giải quyết edge case nào?
8. Task BLOCKED có còn trong ready list không?
9. Remove current task ảnh hưởng index thế nào?
10. Một item có được ở hai list không?

## 8.6. Invariant

- READY task nằm đúng một ready list.
- BLOCKED task không nằm trong ready list.
- List owner trỏ về TCB.
- Container NULL nếu item không thuộc list.
- Không double insert.
- Không double remove.
- Round-robin không trả sentinel.

## 8.7. Host test

- Empty.
- One item.
- Multiple.
- Remove head.
- Remove tail.
- Remove middle.
- Remove current index.
- Double insert.
- Double remove.
- Random operations.
- Forward/backward link check.

## 8.8. Board test

So sánh kết quả scheduler list với scheduler O(n) reference trong debug build.

## 8.9. Tùy chọn: priority bitmap

Chỉ làm khi ready lists đã ổn:

```c
uint32_t ready_bitmap;
```

Tìm highest bằng CLZ.

## 8.10. Điều kiện qua chặng

Scheduler không cần quét toàn bộ TCB trong hot path.

## 8.11. Source tham khảo

- AKOS list + priority.
- FreeRTOS `list.c`, `list.h`.

## 8.12. Tag

```text
v0.4.0-ready-lists
```

---

# CHẶNG 9 — SEMAPHORE VÀ QUEUE

Chặng này chia thành hai phần. Semaphore trước, queue sau.

---

## PHẦN 9A — SEMAPHORE

### Mục tiêu

Task có thể block chờ một event; ISR có thể đánh thức task.

### Data structure

```c
typedef struct
{
    uint32_t count;
    uint32_t max_count;
    ntos_list_t waiting_tasks;
} ntos_semaphore_t;
```

### API

```c
void ntos_sem_init(
    ntos_semaphore_t *sem,
    uint32_t initial_count,
    uint32_t max_count);

ntos_status_t ntos_sem_take(
    ntos_semaphore_t *sem,
    uint32_t timeout_ticks);

ntos_status_t ntos_sem_give(
    ntos_semaphore_t *sem);

ntos_status_t ntos_sem_give_from_isr(
    ntos_semaphore_t *sem,
    bool *higher_priority_task_woken);
```

### Câu hỏi bắt buộc

1. Semaphore dùng để làm gì?
2. Binary và counting khác nhau thế nào?
3. Semaphore có owner không?
4. `take()` khi count 0 làm gì?
5. `give()` khi có waiter làm gì?
6. Chọn waiter nào?
7. Give từ ISR khác task context thế nào?
8. ISR có được block không?
9. `higher_priority_task_woken` có ý nghĩa gì?
10. Context switch xảy ra ngay trong ISR hay sau ISR?
11. Timeout cần thiết tới mức nào trong bản học tập?
12. Nếu chưa làm event timeout phức tạp, có thể giới hạn API ra sao?

### Phạm vi đơn giản hóa

Bản đầu có thể hỗ trợ:

```text
timeout = 0
timeout = WAIT_FOREVER
```

Timeout hữu hạn là tùy chọn.

### Invariant

- `count <= max_count`.
- Waiter phải BLOCKED.
- Waiter không còn trong ready list.
- Give đánh thức task priority cao nhất.
- ISR API không block.
- ISR API không gọi normal API.

### Test

- Give trước take.
- Take trước give.
- No-wait.
- Wait forever.
- Multiple waiters.
- High waiter.
- ISR give.
- Count max.
- Invalid pointer.

### Demo

```text
EXTI button ISR
→ give semaphore
→ worker task wake
→ xử lý trong task context
```

---

## PHẦN 9B — QUEUE

### Mục tiêu

Truyền fixed-size item giữa task và ISR.

### Data structure

```c
typedef struct
{
    uint8_t *storage;
    size_t item_size;
    size_t capacity;
    size_t read_index;
    size_t write_index;
    size_t count;

    ntos_list_t waiting_receivers;
    ntos_list_t waiting_senders;
} ntos_queue_t;
```

### Phạm vi đơn giản hóa

Bắt buộc:

```text
- Static storage
- Fixed item size
- Send task context
- Receive task context
- Send from ISR
- Blocking receive
```

Tùy chọn:

```text
- Blocking send
- Finite timeout
- Peek
- Receive from ISR
```

### API

```c
ntos_status_t ntos_queue_init_static(
    ntos_queue_t *queue,
    void *storage,
    size_t item_size,
    size_t capacity);

ntos_status_t ntos_queue_send(
    ntos_queue_t *queue,
    const void *item,
    uint32_t timeout_ticks);

ntos_status_t ntos_queue_receive(
    ntos_queue_t *queue,
    void *item,
    uint32_t timeout_ticks);

ntos_status_t ntos_queue_send_from_isr(
    ntos_queue_t *queue,
    const void *item,
    bool *higher_priority_task_woken);
```

### Câu hỏi bắt buộc

1. Ring buffer là gì?
2. Full và empty được phân biệt thế nào?
3. Vì sao cần `count`?
4. Send làm thay đổi index nào?
5. Receive làm thay đổi index nào?
6. Queue full thì sender làm gì?
7. Queue empty thì receiver làm gì?
8. Send thành công đánh thức ai?
9. Receive thành công đánh thức ai?
10. Vì sao có hai wait lists?
11. Sau khi task wake, có cần kiểm tra lại điều kiện không?
12. Vì sao ISR không được block?
13. Copy item lớn trong ISR ảnh hưởng gì?
14. Queue khác message queue riêng mỗi task ra sao?

### Invariant

- `count <= capacity`.
- Index luôn trong range.
- Empty iff count = 0.
- Full iff count = capacity.
- Receiver blocked chỉ khi empty.
- Sender blocked chỉ khi full.
- Static storage luôn tồn tại.
- Không malloc.
- ISR send không block.

### Test

- Capacity 1.
- Capacity 2.
- Index wrap.
- Queue full.
- Queue empty.
- Struct item.
- Byte item.
- Multiple receivers.
- Multiple senders nếu có blocking send.
- ISR producer.
- Hàng trăm nghìn operations.

### Demo

```text
USART RX ISR
→ queue_send_from_isr(byte)
→ parser task receive
→ parser xử lý command
```

## 9.3. Điều kiện qua chặng

- Button ISR đánh thức task qua semaphore.
- UART ISR gửi byte cho task qua queue.
- ISR ngắn.
- Không polling.
- Không memory corruption.

## 9.4. Source tham khảo

- AKOS message.
- FreeRTOS queue, chỉ đọc từng flow.
- RT-Thread semaphore/message queue.

## 9.5. Tag

```text
v0.5.0-semaphore-queue
```

---

# CHẶNG 10 — TÍCH HỢP, KIỂM TRA VÀ TỔNG KẾT

## 10.1. Mục tiêu

Hoàn thiện ntos thành một RTOS học tập nhỏ, dễ đọc và có demo tổng hợp.

## 10.2. Không làm gì trong chặng này?

Không thêm các tính năng lớn mới. Không thêm mutex, timer, heap chỉ vì còn thời gian. Chặng này dành cho:

- Sửa invariant.
- Làm sạch API.
- Viết test.
- Viết tài liệu.
- Đo stack.
- Kiểm tra fault.
- Hoàn thiện demo.

## 10.3. Câu hỏi tổng hợp bắt buộc

### Cortex-M

1. MSP và PSP dùng ở đâu?
2. CPU save gì?
3. Kernel save gì?
4. EXC_RETURN làm gì?
5. SVC khác PendSV thế nào?
6. SysTick khác PendSV thế nào?

### Task

1. TCB chứa gì?
2. Initial stack frame dùng để làm gì?
3. Task return được xử lý ra sao?
4. Task state thay đổi thế nào?

### Scheduler

1. Highest-priority READY task được chọn ra sao?
2. Round-robin cùng priority hoạt động thế nào?
3. Yield có cho Low chạy khi High READY không?
4. BLOCKED task nằm ở đâu?
5. Idle task dùng để làm gì?

### Delay

1. Task delay được đánh thức thế nào?
2. Tick overflow được xử lý ra sao?
3. Delay có sai số thế nào?
4. Delay-until khác delay thế nào?

### IPC

1. Semaphore khác queue thế nào?
2. ISR API khác normal API thế nào?
3. Vì sao ISR không block?
4. `higher_priority_task_woken` để làm gì?
5. Queue full/empty được biểu diễn thế nào?

## 10.4. Assert cơ bản

```c
#define NTOS_ASSERT(condition) \
    do { \
        if (!(condition)) { \
            ntos_assert_failed(__FILE__, __LINE__, #condition); \
        } \
    } while (0)
```

Assert handler:

- Disable interrupt.
- Optional UART polling.
- BKPT.
- Infinite loop.

## 10.5. Stack kiểm tra cơ bản

- Fill stack bằng `0xA5`.
- Guard word ở đáy stack.
- Kiểm tra guard khi context switch trong debug build.
- Viết hàm high-water mark đơn giản.

## 10.6. Demo tổng hợp

### Task structure

```text
Control task       High priority
UART parser task   Medium priority
Logger task        Low priority
Idle task          Lowest priority
```

### ISR

```text
EXTI button ISR → semaphore
USART RX ISR     → queue
SysTick          → tick + scheduling
```

### Hoạt động

- Button task được đánh thức.
- UART parser nhận byte.
- Logger định kỳ dùng delay.
- Idle chạy khi không việc.
- Scheduler priority hoạt động.
- Queue/semaphore không polling.

## 10.7. Test tổng hợp

```text
[ ] 1.000.000 context switches.
[ ] Delay qua tick overflow.
[ ] Queue index wrap nhiều lần.
[ ] Semaphore nhiều waiter.
[ ] ISR wake high-priority task.
[ ] Stack guard.
[ ] Task return assert.
[ ] Invalid priority assert.
[ ] Double list insert assert.
[ ] Long-run test ít nhất vài giờ.
```

## 10.8. Tài liệu cuối cùng

```text
README.md
docs/architecture.md
docs/context-switch.md
docs/scheduler.md
docs/task-states.md
docs/ipc.md
docs/testing.md
docs/known-limitations.md
```

Không bắt buộc viết tài liệu so sánh với bất kỳ RTOS nào.

## 10.9. Known limitations phải ghi rõ

```text
- Educational RTOS.
- Không dùng cho safety-critical.
- Không task deletion.
- Static allocation.
- Không MPU.
- Không SMP.
- ISR API giới hạn.
- Timeout có thể chỉ hỗ trợ 0/forever ở IPC.
- Không priority inheritance nếu chưa làm mutex.
- Không software timer nếu không chọn extension.
```

## 10.10. Điều kiện hoàn thành

ntos được coi là hoàn thành khi:

```text
[ ] Task chạy bằng PSP.
[ ] Interrupt dùng MSP.
[ ] SVC start first task.
[ ] PendSV context switch ổn.
[ ] SysTick preemption ổn.
[ ] Priority scheduler đúng.
[ ] Round-robin đúng.
[ ] Delay đúng.
[ ] Idle đúng.
[ ] Ready list đúng.
[ ] Semaphore đúng.
[ ] Queue đúng.
[ ] ISR API đúng.
[ ] Demo tổng hợp chạy.
[ ] Code có tài liệu.
```

## 10.11. Tag

```text
v1.0.0-educational
```

---

# 6. Các extension tùy chọn

## 6.1. Mutex đơn giản

Chỉ làm nếu muốn học priority inversion:

```text
- owner
- lock
- unlock
- chỉ owner unlock
- simple priority inheritance
```

Không cần:

```text
- recursive mutex
- multiple mutex recomputation phức tạp
- transitive inheritance
```

## 6.2. Ordered delayed list

Làm nếu muốn hiểu sâu hơn cách RTOS tránh quét toàn bộ task.

## 6.3. Dual delayed lists

Làm nếu muốn hiểu FreeRTOS tick overflow.

## 6.4. Software timer đơn giản

Một timer task và ordered timer list, không cần command queue quá phức tạp.

## 6.5. Runtime stats

Đếm thời gian task bằng hardware timer.

---

# 7. Bảng tiến độ

| Chặng | Nội dung | Code | Test | Docs | Tag |
|---:|---|:---:|:---:|:---:|:---:|
| 0 | Bare-metal | [ ] | [ ] | [ ] | [ ] |
| 1 | MSP/PSP/Exception | [ ] | [ ] | [ ] | [ ] |
| 2 | TCB/Initial stack | [ ] | [ ] | [ ] | [ ] |
| 3 | First task/SVC | [ ] | [ ] | [ ] | [ ] |
| 4 | Cooperative/PendSV | [ ] | [ ] | [ ] | [ ] |
| 5 | Preemptive/SysTick | [ ] | [ ] | [ ] | [ ] |
| 6 | State/Priority/Idle | [ ] | [ ] | [ ] | [ ] |
| 7 | Delay | [ ] | [ ] | [ ] | [ ] |
| 8 | Ready list | [ ] | [ ] | [ ] | [ ] |
| 9 | Semaphore/Queue | [ ] | [ ] | [ ] | [ ] |
| 10 | Integration | [ ] | [ ] | [ ] | [ ] |

---

# 8. Lộ trình 14 ngày gợi ý

| Ngày | Nội dung |
|---:|---|
| 1 | Bare-metal + ôn MSP/PSP |
| 2 | Exception experiments |
| 3 | TCB + initial stack |
| 4 | Start first task |
| 5 | Cooperative PendSV |
| 6 | Stress context switch |
| 7 | SysTick preemption |
| 8 | Task states + idle |
| 9 | Priority scheduler O(n) |
| 10 | Delay |
| 11 | Ready list |
| 12 | Semaphore |
| 13 | Queue |
| 14 | Integration + docs + tests |

Không bắt buộc hoàn thành trong 14 ngày. Mục tiêu là không bỏ qua stress test của context switch.

---

# 9. Câu nhắc quan trọng nhất

> Mỗi tính năng kernel phải được mô tả bằng ba câu:
>
> 1. Task đang ở state nào?
> 2. Task đang nằm trong cấu trúc nào?
> 3. Sự kiện nào khiến PendSV được yêu cầu?

Nếu chưa trả lời được ba câu này, chưa nên code tính năng đó.
