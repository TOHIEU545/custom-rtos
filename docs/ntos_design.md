# NTOS DESIGN DOCUMENT

> **Project:** ntos — Nghĩa Trụ Operating System  
> **Loại:** Educational preemptive RTOS  
> **Target:** STM32F103 / ARM Cortex-M3  
> **Mục tiêu thiết kế:** Nhỏ, rõ, dễ đọc, dễ debug, thể hiện đúng bản chất RTOS.  
> **Không phải mục tiêu:** Production-grade feature set, safety certification, API compatibility với FreeRTOS.

---

# 1. Mục đích của tài liệu thiết kế

Tài liệu này là “hợp đồng kiến trúc” của ntos. Khi code có xu hướng phức tạp dần, tài liệu giúp trả lời:

- Module nào chịu trách nhiệm?
- State nào hợp lệ?
- Task nằm trong cấu trúc nào?
- Context switch xảy ra ở đâu?
- API nào được gọi từ ISR?
- Dữ liệu nào phải được bảo vệ?
- Tính năng nào cố ý không hỗ trợ?

Nếu code khác tài liệu, phải làm một trong hai:

1. Sửa code để đúng thiết kế.
2. Cập nhật tài liệu và ghi rõ lý do thay đổi.

---

# 2. Mục tiêu thiết kế

## 2.1. Ưu tiên

Theo thứ tự:

1. **Đúng bản chất.**
2. **Dễ hiểu.**
3. **Dễ kiểm tra bằng debugger.**
4. **Không phụ thuộc heap.**
5. **Ít abstraction không cần thiết.**
6. **Hiệu năng đủ cho project học tập.**

## 2.2. Không ưu tiên

- Tối ưu từng cycle.
- Tương thích nhiều CPU.
- Tương thích nhiều compiler.
- Hỗ trợ nhiều kiểu IPC.
- Runtime object creation/deletion.
- Feature parity với FreeRTOS.

---

# 3. Tổng quan kiến trúc

```text
+--------------------------------------------------+
|                  Application                     |
|  Control task | UART task | Logger task | Idle  |
+-------------------------+------------------------+
                          |
                          v
+--------------------------------------------------+
|                  NTOS Public API                  |
| Task | Delay | Semaphore | Queue | Diagnostics   |
+--------------------------------------------------+
                          |
                          v
+--------------------------------------------------+
|                    NTOS Kernel                    |
| Scheduler | Task State | Ready Lists | Blocking  |
| Tick      | Semaphore  | Queue       | Assertions|
+--------------------------------------------------+
                          |
                          v
+--------------------------------------------------+
|                  Cortex-M3 Port                   |
| SVC | PendSV | SysTick | PSP/MSP | IRQ control   |
+--------------------------------------------------+
                          |
                          v
+--------------------------------------------------+
|                 STM32F103 BSP                     |
| Clock | GPIO | UART | EXTI | Startup | Linker    |
+--------------------------------------------------+
```

---

# 4. Cấu trúc thư mục

```text
ntos/
├── include/
│   ├── ntos.h
│   ├── ntos_config.h
│   ├── ntos_types.h
│   ├── ntos_task.h
│   ├── ntos_sem.h
│   ├── ntos_queue.h
│   └── ntos_status.h
│
├── kernel/
│   ├── ntos_kernel.c
│   ├── ntos_task.c
│   ├── ntos_list.c
│   ├── ntos_sem.c
│   ├── ntos_queue.c
│   ├── ntos_assert.c
│   └── internal/
│       ├── ntos_kernel_internal.h
│       ├── ntos_task_internal.h
│       └── ntos_list_internal.h
│
├── port/
│   └── cortex_m3/
│       ├── ntos_port.c
│       ├── ntos_port.h
│       └── ntos_port_asm.S
│
├── bsp/
│   └── stm32f103/
│       ├── startup_stm32f103.S
│       ├── linker.ld
│       ├── system_stm32f103.c
│       ├── gpio.c
│       ├── uart.c
│       └── board.h
│
├── examples/
├── tests/
└── docs/
```

---

# 5. Phân chia trách nhiệm module

## 5.1. `ntos_task.c`

Chịu trách nhiệm:

- Tạo task.
- Initialize TCB.
- Dựng initial stack.
- Task states.
- Delay.
- Add/remove task khỏi ready structure.
- Chọn task tiếp theo ở mức logic.

Không chứa:

- Inline assembly.
- Truy cập PSP/MSP trực tiếp.
- Truy cập SCB/NVIC trực tiếp.

## 5.2. `ntos_kernel.c`

Chịu trách nhiệm:

- Kernel initialization.
- Scheduler start.
- Tick processing.
- Current task.
- Scheduler running state.
- Yêu cầu reschedule.
- Idle task creation.

## 5.3. `ntos_list.c`

Chịu trách nhiệm:

- Ready list.
- Wait list.
- Insert/remove.
- Round-robin cursor.
- List invariant checks.

## 5.4. `ntos_sem.c`

Chịu trách nhiệm:

- Semaphore count.
- Waiting task list.
- Take/give.
- Give from ISR.
- Wake highest-priority waiter.

## 5.5. `ntos_queue.c`

Chịu trách nhiệm:

- Ring buffer.
- Copy item.
- Full/empty logic.
- Waiting sender/receiver.
- Send/receive.
- Send from ISR.

## 5.6. Cortex-M3 port

Chịu trách nhiệm:

- Start first task.
- SVC.
- PendSV.
- SysTick hardware setup.
- PSP/MSP.
- CONTROL.
- PRIMASK.
- NVIC priority.
- Pend context switch.

## 5.7. BSP

Chịu trách nhiệm:

- Clock.
- Board pin.
- UART.
- EXTI.
- Startup.
- Linker.
- Không chứa scheduler logic.

## 5.8. Quy ước đặt tên hàm

### Public API

Public API (khai báo trong `include/ntos/*.h`, dùng được từ ngoài module) dùng tiền tố `ntos_`:

```c
ntos_<module>_<action>()
```

Ví dụ: `ntos_task_create_static()`, `ntos_yield()`, `ntos_delay()`.

### Static helper nội bộ một file `.c`

Hàm `static` chỉ dùng trong một file `.c` **bỏ tiền tố `ntos_`**, nhưng vẫn giữ tên module ở đầu để không quá chung chung:

```c
<module>_<action>()
```

Ví dụ trong `kernel/ntos_task.c`:

- `task_return_error()`
- `task_build_initial_stack()`
- `task_validate_create_arguments()`

Không dùng tên chung chung không gắn module, ví dụ `init()`, `validate()`, `setup()`.

Lý do:

- Nhìn tên phân biệt ngay đâu là public API (`ntos_...`), đâu là helper nội bộ file (`<module>_...`).
- Tên ngắn hơn `ntos_<module>_...` nhưng vẫn đủ rõ ràng thuộc module nào nhờ tiền tố `<module>_`.

---

# 6. Cấu hình compile-time

`ntos_config.h`:

```c
#define NTOS_TICK_RATE_HZ          1000U
#define NTOS_MAX_TASKS             8U
#define NTOS_MAX_PRIORITIES        8U
#define NTOS_IDLE_STACK_WORDS      128U

#define NTOS_ENABLE_ASSERT         1
#define NTOS_ENABLE_STACK_FILL     1
#define NTOS_ENABLE_STACK_GUARD    1
#define NTOS_ENABLE_WFI_IDLE       0
#define NTOS_ENABLE_PRIORITY_MAP   0
```

Quy tắc:

- Số priority nhỏ để dễ debug.
- Không dùng config quá nhiều ở giai đoạn đầu.
- Chỉ thêm config khi có hai cách triển khai thực sự cần lựa chọn.

---

# 7. Kiểu dữ liệu chung

## 7.1. Tick

```c
typedef uint32_t ntos_tick_t;
```

## 7.2. Priority

```c
typedef uint8_t ntos_priority_t;
```

## 7.3. Status

```c
typedef enum
{
    NTOS_OK = 0,
    NTOS_ERR_INVALID_ARGUMENT,
    NTOS_ERR_INVALID_STATE,
    NTOS_ERR_INVALID_PRIORITY,
    NTOS_ERR_INVALID_CONTEXT,
    NTOS_ERR_TIMEOUT,
    NTOS_ERR_QUEUE_FULL,
    NTOS_ERR_QUEUE_EMPTY,
    NTOS_ERR_RESOURCE_FULL,
    NTOS_ERR_INTERNAL
} ntos_status_t;
```

---

# 8. Task model

## 8.1. TCB đề xuất

```c
typedef void (*ntos_task_entry_t)(void *argument);

typedef enum
{
    NTOS_TASK_UNUSED = 0,
    NTOS_TASK_READY,
    NTOS_TASK_RUNNING,
    NTOS_TASK_BLOCKED
} ntos_task_state_t;

typedef struct ntos_tcb
{
    uint32_t *stack_pointer;

    uint32_t *stack_low;
    uint32_t *stack_high;
    size_t stack_words;

    ntos_task_entry_t entry;
    void *argument;
    const char *name;

    ntos_priority_t priority;
    ntos_task_state_t state;

    ntos_tick_t wake_tick;

    ntos_list_item_t ready_item;
    ntos_list_item_t wait_item;
} ntos_tcb_t;
```

## 8.2. Tại sao `stack_pointer` ở đầu?

PendSV có thể lưu PSP trực tiếp:

```asm
STR r0, [r1]
```

nếu `r1` trỏ tới TCB.

## 8.3. Vì sao hai list item?

- `ready_item`: membership trong ready list hoặc state-related list.
- `wait_item`: membership trong semaphore/queue wait list.

Trong phiên bản đơn giản, task delay có thể chỉ dùng `state + wake_tick`, chưa cần delayed list.

## 8.4. Task lifetime

```text
Static TCB
Static stack
Create before ntos_start()
No deletion
Lifetime = toàn bộ chương trình
```

Điều này giúp tránh:

- Dangling TCB.
- Stack free sai.
- Scheduler cleanup.
- Idle cleanup logic.
- Heap fragmentation.

---

# 9. Stack model

## 9.1. MSP

Dùng cho:

- Reset.
- Main trước khi scheduler start.
- SVC.
- PendSV.
- SysTick.
- Driver ISR.
- HardFault.

## 9.2. PSP

Dùng cho:

- Application tasks.
- Idle task.

Mỗi task có PSP riêng được lưu trong TCB.

## 9.3. Initial stack frame

```text
High address

Hardware-compatible frame:
xPSR
PC
LR
R12
R3
R2
R1
R0

Software-saved frame:
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

## 9.4. Task return policy

Task function không được return.

```c
static void ntos_task_return_error(void)
{
    NTOS_ASSERT(false);

    for (;;)
    {
    }
}
```

---

# 10. Context switch design

## 10.1. Nguyên tắc

**Chỉ PendSV thực hiện context switch.**

Các nơi khác chỉ request:

```text
ntos_yield()
SysTick
semaphore give
queue send
task delay
```

## 10.2. Cooperative switch

```text
Task A
  |
ntos_yield
  |
PendSV pending
  |
PendSV save A
  |
scheduler selects B
  |
PendSV restore B
  |
Task B
```

## 10.3. Preemptive switch

```text
SysTick
  |
increment tick
  |
wake delayed tasks
  |
decide reschedule
  |
pend PendSV
  |
return from SysTick
  |
PendSV runs
```

## 10.4. PendSV pseudo assembly

```asm
PendSV_Handler:
    MRS     r0, psp
    STMDB   r0!, {r4-r11}

    LDR     r1, =ntos_current_tcb
    LDR     r2, [r1]
    STR     r0, [r2]

    PUSH    {lr}
    BL      ntos_scheduler_select_next
    POP     {lr}

    LDR     r1, =ntos_current_tcb
    LDR     r2, [r1]
    LDR     r0, [r2]

    LDMIA   r0!, {r4-r11}
    MSR     psp, r0
    BX      lr
```

Assembly thật phải kiểm tra ABI và stack alignment.

## 10.5. SVC first task flow

```text
main using MSP
→ ntos_start
→ select first task
→ SVC
→ restore R4-R11
→ set PSP
→ Thread mode uses PSP
→ exception return
→ task starts
```

---

# 11. Scheduler design

## 11.1. Policy

```text
Fixed-priority preemptive
Round-robin among equal priorities
Idle at priority 0
Higher numeric value = higher priority
```

## 11.2. Scheduler rules

1. Chỉ chọn task READY.
2. Priority cao nhất được chạy.
3. Task cùng priority luân phiên.
4. High task vừa READY preempt Low.
5. Low task vừa READY không preempt High.
6. Yield không cho Low chạy nếu High khác vẫn READY.
7. Idle chỉ chạy khi không task application READY.

## 11.3. Giai đoạn đầu: O(n)

```c
ntos_tcb_t *ntos_scheduler_select_highest_ready(void);
```

Quét task table.

## 11.4. Giai đoạn cuối: ready lists

```c
static ntos_list_t ready_lists[NTOS_MAX_PRIORITIES];
```

Mỗi list quản lý task cùng priority.

## 11.5. Round-robin

List có `index`.

Mỗi lần cần task mới cùng priority:

```text
index = index->next
skip sentinel
return owner
```

## 11.6. Priority bitmap tùy chọn

```c
static uint32_t ready_bitmap;
```

Không bắt buộc.

---

# 12. Task state model

## 12.1. State diagram

```text
             create
               |
               v
             READY
               |
          scheduled
               v
            RUNNING
          /    |    \
       yield   |    delay/wait
        /      |       \
       v       |        v
     READY <---+      BLOCKED
                         |
                   event/timeout
                         |
                         v
                       READY
```

## 12.2. State membership

| State | Ready list | Wait list | Có thể chạy |
|---|:---:|:---:|:---:|
| READY | Có | Không | Có |
| RUNNING | Theo thiết kế, vẫn có thể ở ready list | Không | Đang chạy |
| BLOCKED delay | Không | Không | Không |
| BLOCKED semaphore | Không | Có | Không |
| BLOCKED queue | Không | Có | Không |

## 12.3. Invariant

- READY task không ở wait list.
- BLOCKED task không ở ready list.
- Current task hợp lệ.
- Idle luôn READY.
- Task state và list membership phải khớp.

---

# 13. Tick và delay design

## 13.1. Tick source

SysTick ở 1 kHz:

```text
1 tick = 1 ms
```

## 13.2. Tick handler responsibilities

- Increment tick.
- Wake delayed task.
- Decide time-slice.
- Pend PendSV nếu cần.

Không làm:

- UART print.
- Callback dài.
- Queue parsing.
- Dynamic allocation.

## 13.3. Delay representation

TCB:

```c
ntos_tick_t wake_tick;
```

Khi delay:

```text
remove current from ready
state = BLOCKED
wake_tick = now + delay
request reschedule
```

Khi tick:

```text
for each task:
    if BLOCKED by delay and expired:
        state = READY
        add ready list
```

## 13.4. Tick overflow

Helper:

```c
static bool ntos_tick_reached(ntos_tick_t now, ntos_tick_t target)
{
    return (int32_t)(now - target) >= 0;
}
```

Giới hạn delay:

```text
delay <= INT32_MAX ticks
```

## 13.5. Delay zero

Chọn một semantics và giữ nhất quán.

Khuyến nghị:

```text
ntos_delay(0) == ntos_yield()
```

---

# 14. Ready list design

## 14.1. List item

```c
typedef struct ntos_list_item
{
    struct ntos_list_item *next;
    struct ntos_list_item *previous;
    void *owner;
    struct ntos_list *container;
} ntos_list_item_t;
```

## 14.2. List

```c
typedef struct ntos_list
{
    ntos_list_item_t end;
    ntos_list_item_t *index;
    uint32_t length;
} ntos_list_t;
```

## 14.3. Required operations

```c
void ntos_list_init(ntos_list_t *list);
void ntos_list_item_init(ntos_list_item_t *item, void *owner);
void ntos_list_push_back(ntos_list_t *list, ntos_list_item_t *item);
void ntos_list_remove(ntos_list_item_t *item);
void *ntos_list_next_owner(ntos_list_t *list);
bool ntos_list_is_empty(const ntos_list_t *list);
```

## 14.4. Invariants

- Sentinel link luôn hợp lệ.
- Length đúng.
- Container đúng.
- Không double insert.
- Không double remove.
- Index không trỏ vào item đã remove.
- Owner của task item là TCB.

---

# 15. Semaphore design

## 15.1. Mục tiêu

Đồng bộ event mà không truyền payload.

## 15.2. Structure

```c
typedef struct
{
    uint32_t count;
    uint32_t max_count;
    ntos_list_t waiting_tasks;
} ntos_semaphore_t;
```

## 15.3. Take

```text
enter critical

if count > 0:
    count--
    leave critical
    return OK

if no wait:
    leave critical
    return TIMEOUT

remove current from ready
state = BLOCKED
insert current.wait_item into semaphore wait list

leave critical
request PendSV
```

## 15.4. Give

Khuyến nghị direct handoff:

```text
enter critical

if waiter exists:
    remove highest waiter
    set READY
    add ready list
    determine whether preemption needed
else if count < max_count:
    count++
else:
    return RESOURCE_FULL

leave critical
request PendSV if needed
```

## 15.5. Wait list ordering

Có hai lựa chọn:

1. Sort theo priority.
2. Mỗi lần give quét wait list tìm highest.

Vì project nhỏ, có thể quét để code dễ hiểu.

## 15.6. ISR give

Khác normal give:

- Không block.
- Không gọi scheduler trực tiếp.
- Trả `higher_priority_task_woken`.
- Caller yêu cầu yield-from-ISR.

---

# 16. Queue design

## 16.1. Mục tiêu

Truyền fixed-size item bằng static ring buffer.

## 16.2. Structure

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

## 16.3. Memory layout

Item `i`:

```c
storage + i * item_size
```

## 16.4. Send

```text
if not full:
    copy to write_index
    write_index = (write_index + 1) % capacity
    count++
    wake receiver if any
else:
    no-wait -> QUEUE_FULL
    blocking -> block sender
```

## 16.5. Receive

```text
if not empty:
    copy from read_index
    read_index = (read_index + 1) % capacity
    count--
    wake sender if any
else:
    no-wait -> QUEUE_EMPTY
    blocking -> block receiver
```

## 16.6. Simplification policy

Bản ntos bắt buộc:

- Blocking receive.
- Non-blocking hoặc blocking send tùy tiến độ.
- ISR send.

Không bắt buộc:

- Queue peek.
- Queue set.
- Zero-copy.
- Variable-size messages.
- Receive from ISR.

## 16.7. Important race

Sau khi một task được wake, task khác có thể chạy trước và thay đổi queue. Do đó blocking operation phải kiểm tra lại điều kiện sau khi resume.

---

# 17. Interrupt model

## 17.1. Giai đoạn đầu

Dùng PRIMASK save/restore:

```c
typedef uint32_t ntos_irq_state_t;

ntos_irq_state_t ntos_irq_save(void);
void ntos_irq_restore(ntos_irq_state_t state);
```

## 17.2. Quy tắc

- Blocking API chỉ gọi từ task context.
- ISR dùng API có hậu tố `_from_isr`.
- ISR API không block.
- ISR API không dùng heap.
- ISR API chỉ set flag yêu cầu context switch.
- PendSV thực hiện switch sau ISR.

## 17.3. Critical section

Chỉ bảo vệ:

- Ready list.
- Task state.
- Semaphore count/wait list.
- Queue count/index/wait list.
- Current scheduling metadata.

Không bao quanh:

- UART output.
- Callback dài.
- Memory copy lớn nếu có thể tránh.
- Busy loop.

## 17.4. BASEPRI

Không bắt buộc trong core educational version. Có thể thêm extension sau khi PRIMASK implementation đã đúng.

---

# 18. Idle task design

## 18.1. Nhiệm vụ

- Đảm bảo scheduler luôn có task.
- Chạy khi không application task READY.
- Có thể WFI.
- Có thể kiểm tra stack guard trong debug.

## 18.2. Không làm

- Không block.
- Không dùng queue.
- Không giữ semaphore.
- Không chạy logic application.

---

# 19. Assert và diagnostics

## 19.1. Assert

```c
#define NTOS_ASSERT(expr) \
    do { \
        if (!(expr)) { \
            ntos_assert_failed(__FILE__, __LINE__, #expr); \
        } \
    } while (0)
```

## 19.2. Assert handler

```text
disable interrupts
store file/line/expression
optional UART polling
BKPT
loop forever
```

## 19.3. Các assert quan trọng

- TCB pointer.
- Stack pointer range.
- Priority range.
- List item container.
- Queue count.
- Queue index.
- Semaphore count.
- API context.
- Current task non-NULL.
- Idle READY.
- Task return.

## 19.4. Stack diagnostics

- Fill `0xA5`.
- Guard word.
- High-water mark.
- Optional PSP bounds in PendSV debug build.

---

# 20. Public API draft

## 20.1. Kernel/task

```c
void ntos_init(void);

ntos_status_t ntos_task_create_static(
    ntos_tcb_t *tcb,
    const char *name,
    ntos_task_entry_t entry,
    void *argument,
    uint32_t *stack,
    size_t stack_words,
    ntos_priority_t priority);

void ntos_start(void) __attribute__((noreturn));
void ntos_yield(void);
void ntos_delay(ntos_tick_t ticks);
ntos_tick_t ntos_tick_get(void);
```

## 20.2. Semaphore

```c
void ntos_sem_init(
    ntos_semaphore_t *sem,
    uint32_t initial_count,
    uint32_t max_count);

ntos_status_t ntos_sem_take(
    ntos_semaphore_t *sem,
    ntos_tick_t timeout);

ntos_status_t ntos_sem_give(
    ntos_semaphore_t *sem);

ntos_status_t ntos_sem_give_from_isr(
    ntos_semaphore_t *sem,
    bool *higher_priority_task_woken);
```

## 20.3. Queue

```c
ntos_status_t ntos_queue_init_static(
    ntos_queue_t *queue,
    void *storage,
    size_t item_size,
    size_t capacity);

ntos_status_t ntos_queue_send(
    ntos_queue_t *queue,
    const void *item,
    ntos_tick_t timeout);

ntos_status_t ntos_queue_receive(
    ntos_queue_t *queue,
    void *item,
    ntos_tick_t timeout);

ntos_status_t ntos_queue_send_from_isr(
    ntos_queue_t *queue,
    const void *item,
    bool *higher_priority_task_woken);
```

---

# 21. Timeout policy

Để giữ project đơn giản, đề xuất hai giai đoạn.

## Giai đoạn 1

```c
#define NTOS_NO_WAIT      0U
#define NTOS_WAIT_FOREVER UINT32_MAX
```

Semaphore/queue chỉ hỗ trợ:

- Không chờ.
- Chờ vô hạn.

## Giai đoạn 2 tùy chọn

Thêm finite timeout bằng `wake_tick`.

Lợi ích của chia hai giai đoạn:

- Học blocking trước.
- Tránh event-timeout race quá sớm.
- Kernel vẫn có giá trị học tập.

---

# 22. Scheduler decision table

| Event | Current | Task affected | Decision |
|---|---:|---:|---|
| Tick time slice | P | P | Switch round-robin |
| High task wakes | Low | High | PendSV |
| Low task wakes | High | Low | No immediate switch |
| Current delays | Any | Current | PendSV |
| Current waits semaphore | Any | Current | PendSV |
| Queue wakes higher receiver | Low | High | PendSV |
| ISR wakes higher task | Low | High | Set wake flag, PendSV after ISR |
| Yield | P | Same P available | Round-robin |
| Yield | High | Only Low available | Continue High |

---

# 23. Kernel invariants

Đây là phần quan trọng nhất của tài liệu.

## 23.1. Task invariants

```text
- Mỗi task có một stack riêng.
- stack_pointer nằm trong vùng stack.
- Task READY nằm trong ready structure.
- Task BLOCKED không nằm trong ready structure.
- Current task không NULL sau scheduler start.
- Idle task luôn tồn tại.
```

## 23.2. List invariants

```text
- Item thuộc tối đa một list qua một list field.
- container phản ánh đúng membership.
- length đúng.
- next/previous đối xứng.
- sentinel không bị remove.
```

## 23.3. Scheduler invariants

```text
- Scheduler chỉ chọn READY task.
- Không chạy Low nếu High READY.
- Equal-priority task được round-robin.
- PendSV là nơi duy nhất đổi current task context.
```

## 23.4. Queue invariants

```text
0 <= count <= capacity
read_index < capacity
write_index < capacity
count == 0 => empty
count == capacity => full
```

## 23.5. Semaphore invariants

```text
0 <= count <= max_count
wait list chỉ chứa BLOCKED tasks
ISR give không block
```

---

# 24. Testing strategy

## 24.1. Host tests

- List.
- Ring buffer helper.
- Tick arithmetic.
- Priority selection.
- Queue index wrap.

## 24.2. Target tests

- Register preservation.
- PSP/MSP.
- Context switch stress.
- Delay.
- Priority.
- Semaphore.
- Queue.
- ISR wake.

## 24.3. Fault injection

- Task return.
- Invalid priority.
- Stack guard corruption.
- Double list insert.
- Queue count corruption trong debug test.
- Semaphore over-give.
- Invalid ISR context use.

## 24.4. Long-run test

Chạy:

```text
- 4 application tasks
- SysTick
- UART RX ISR
- EXTI ISR
- queue traffic
- semaphore traffic
```

trong vài giờ.

---

# 25. Demo application design

## Tasks

### Control task

- Priority cao.
- Chờ semaphore từ button.
- Xử lý nhanh rồi block lại.

### UART parser task

- Priority trung bình.
- Chờ queue byte.
- Parse command.

### Logger task

- Priority thấp.
- Delay định kỳ.
- In task state đơn giản.

### Idle task

- Priority thấp nhất.
- Optional WFI.

## Interrupts

### EXTI

```text
clear flag
give semaphore from ISR
request switch if needed
```

### USART RX

```text
read byte
send queue from ISR
request switch if needed
```

### SysTick

```text
increment tick
wake delay
time slice
pend PendSV if needed
```

---

# 26. Known limitations

```text
- Chỉ Cortex-M3.
- Chỉ GCC.
- Static tasks.
- Không task deletion.
- Không MPU.
- Không SMP.
- Không FPU.
- IPC timeout hữu hạn có thể chưa hỗ trợ.
- PRIMASK có thể tăng interrupt latency.
- Không production safety.
- Không deadlock detection.
- Không priority inheritance nếu mutex extension chưa làm.
- Không software timer trong core version.
```

---

# 27. Những quyết định cố ý đơn giản hóa

## 27.1. Static tasks

Vì mục tiêu là hiểu scheduler, không phải heap.

## 27.2. O(n) trước list

Vì cần hiểu policy trước optimization.

## 27.3. PRIMASK trước BASEPRI

Vì dễ chứng minh đúng hơn.

## 27.4. Delay scan O(n)

Vì số task nhỏ và dễ hiểu.

## 27.5. Semaphore/queue timeout đơn giản

Vì event-timeout race là chủ đề nâng cao.

## 27.6. Không task deletion

Vì task deletion kéo theo cleanup, ownership và memory lifetime.

---

# 28. Tiêu chí thiết kế thành công

Thiết kế ntos được coi là thành công nếu:

1. Một người đọc có thể hiểu context switch từ tài liệu.
2. Mỗi task state có membership rõ.
3. Mọi reschedule đều đi qua PendSV.
4. Không cần heap để chạy demo.
5. Semaphore và queue thể hiện đúng blocking.
6. ISR chỉ làm việc ngắn.
7. Code nhỏ và module rõ.
8. Có thể debug bằng GDB mà không phụ thuộc tool đặc biệt.
9. Mọi giới hạn được ghi rõ.
10. Project dạy được bản chất RTOS thay vì chỉ “chạy được”.

---

# 29. Câu hỏi kiểm tra thiết kế trước khi merge code

Mỗi pull request hoặc commit lớn phải trả lời:

```text
1. Task state nào thay đổi?
2. Task rời cấu trúc nào?
3. Task vào cấu trúc nào?
4. Dữ liệu nào cần critical section?
5. Có cần PendSV không?
6. Có thể được gọi từ ISR không?
7. API có thể block không?
8. Invariant nào mới?
9. Test nào chứng minh?
10. Có làm ntos phức tạp hơn mục tiêu học tập không?
```

---

# 30. Quy tắc cuối cùng

> ntos không cần nhiều tính năng. ntos cần làm rõ ba bản chất:
>
> 1. **CPU context được lưu và khôi phục thế nào.**
> 2. **Scheduler quyết định task nào được chạy thế nào.**
> 3. **Task chuyển READY/BLOCKED và được đánh thức thế nào.**
>
> Mọi module khác chỉ tồn tại để minh họa ba bản chất này.
