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

> **Trạng thái thực tế (Chặng 5):** `ntos_port_start_tick()` (port/cortex_m3/ntos_port.c)
> set priority PendSV/SysTick về thấp nhất rồi gọi `SysTick_Config()` (CMSIS) ở
> `NTOS_TICK_RATE_HZ`. `SysTick_Handler` chỉ gọi `ntos_tick_handler()` (kernel) -
> không tự tăng tick, không tự đổi `ntos_current_tcb`, không tự pend PendSV.
> `SVC_Handler` tự set `ntos_scheduler_running = true` ngay sau `msr psp, r0` (PSP
> đã hợp lệ) — không phải `ntos_start()` set trước khi gọi SVC — để loại bỏ khả
> năng SysTick pend PendSV khi PSP chưa sẵn sàng, kể cả trong khe hở giữa
> `__enable_irq()` và lệnh `svc` trong `ntos_port_start_scheduler()`.

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

TCB là type **công khai**, định nghĩa tại `include/ntos/ntos_task.h` (không phải
opaque, không phải ở header nội bộ) — để application có thể khai báo TCB tĩnh
trực tiếp, ví dụ `static ntos_tcb_t task1_tcb;`. `kernel/internal/ntos_task_internal.h`
chỉ `#include <ntos/ntos_task.h>` để code nội bộ kernel dùng chung type này.

TCB tối thiểu (đúng trạng thái từ Chặng 2 trở đi):

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
} ntos_tcb_t;
```

> **Trạng thái thực tế (Chặng 9A):** TCB hiện tại (`include/ntos/ntos_task.h`)
> đã có đủ `wake_tick` (Chặng 7), `ready_item` (Chặng 8) **và `wait_item`**
> (Chặng 9A) so với bản tối thiểu ở trên. Xem §8.3 để phân biệt vai trò
> `ready_item` (membership trong `ready_lists[]`) và `wait_item` (membership
> trong `waiting_tasks` của semaphore) - hai field độc lập, không dùng
> `ready_item` cho wait list. **Có thêm field `block_reason`** (kiểu
> `ntos_block_reason_t`: `NTOS_BLOCK_NONE`/`NTOS_BLOCK_DELAY`/
> `NTOS_BLOCK_SEMAPHORE`/`NTOS_BLOCK_QUEUE_SEND`/`NTOS_BLOCK_QUEUE_RECEIVE`) -
> thêm sau khi sửa một bug thật phát hiện trên board (xem §13.2): chỉ dựa vào
> `state == NTOS_TASK_BLOCKED` không đủ để `ntos_tick_handler()` biết có được
> wake task hay không, vì task BLOCKED vì IPC cũng thoả điều kiện đó.

## 8.2. Tại sao `stack_pointer` ở đầu?

PendSV có thể lưu PSP trực tiếp:

```asm
STR r0, [r1]
```

nếu `r1` trỏ tới TCB.

## 8.3. Vì sao hai list item?

TCB có hai list item độc lập, mỗi field chỉ thuộc tối đa một list tại một thời điểm:

- `ready_item`: membership trong `ready_lists[priority]` (Chặng 8) - mọi task
  RUNNING/READY nằm ở đây, `container == NULL` khi BLOCKED/UNUSED.
- `wait_item`: membership trong `waiting_tasks` của semaphore/queue (Chặng 9A
  trở đi) - chỉ có ý nghĩa khi task BLOCKED vì chờ event, `container == NULL`
  khi task không chờ semaphore/queue nào.

Invariant: một task RUNNING/READY có `ready_item.container != NULL` và
`wait_item.container == NULL`; một task BLOCKED vì semaphore có
`ready_item.container == NULL` và `wait_item.container != NULL`. Task delay
(Chặng 7, chờ tick chứ không chờ semaphore) chỉ dùng `state + wake_tick`,
không đụng tới `wait_item`.

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

> **Trạng thái thực tế (Chặng 6):** `ntos_scheduler_select_next()` (kernel/ntos_kernel.c)
> làm đúng O(n) 2-pass trong `task_table[]` có sẵn (không dùng ready list riêng
> như §11.4/§11.5 - để dành cho Chặng 8): Pass 1 tìm `highest_ready_priority`
> trong các task đang READY; Pass 2 round-robin từ `current_task_index`, chỉ
> dừng ở task READY có đúng priority đó. **Quyết định:** idle (priority 0) và
> một app task cũng ở priority 0 (nếu có) sẽ round-robin bình thường với nhau -
> không có special-case nào dành riêng cho idle trong scheduler, giữ đúng thiết
> kế fixed-priority thuần nhất đã có. Nếu ai đó không muốn idle bị chia sẻ CPU,
> chỉ cần không tạo task application ở priority 0.

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

> **Trạng thái thực tế (Chặng 5):** `ntos_tick_handler()` (kernel/ntos_kernel.c)
> mới chỉ tăng `ntos_tick_count` và pend PendSV nếu `ntos_scheduler_running`.
> Wake delayed task/time-slice theo priority là việc của Chặng 6/7, chưa
> implement - guard duy nhất hiện có là `ntos_scheduler_running` để SysTick
> không context-switch trước khi first task start xong.

> **Bug thật phát hiện trên board (Chặng 9A, sửa cùng lúc thêm `block_reason`):**
> Điều kiện wake ban đầu của Chặng 7 chỉ kiểm `state == NTOS_TASK_BLOCKED`, không
> phân biệt task đang BLOCKED vì lý do gì. Một task gọi `ntos_sem_take()`/
> `ntos_queue_send()`/`ntos_queue_receive()` (WAIT_FOREVER) mà **chưa từng gọi
> `ntos_delay()`** thì `wake_tick` vẫn còn giá trị khởi tạo `0`, và
> `ntos_tick_reached(now, 0)` = `(int32_t)now >= 0` **gần như luôn đúng** (từ
> tick 0 tới ~`INT32_MAX`, tức ~24 ngày ở 1 kHz) - nên `ntos_tick_handler()` sẽ
> "wake giả" task đó ngay SysTick kế tiếp, đẩy `ready_item` vào `ready_lists[]`
> **mà không hề gỡ `wait_item` khỏi `waiting_tasks`/`waiting_receivers`/
> `waiting_senders`** (đoạn code này không biết gì về IPC wait list). Kết quả:
> task vừa nằm trong ready list vừa nằm trong wait list cùng lúc - vi phạm
> invariant §8.3, lộ ra dưới dạng `NTOS_ASSERT` fail ở `ntos_sem_take()` khi
> task đó block lần tiếp theo (bắt được thật khi chạy `chang09_semaphore_isr`
> trên board, xem GDB watch: `ready_item.container` và `wait_item.container`
> cùng khác `NULL`). **Sửa bằng field mới `block_reason`** (`ntos_block_reason_t`,
> `include/ntos/ntos_task.h`) thay vì suy luận qua `wait_item.container == NULL`,
> vì cách suy luận đó sẽ sai ngay khi Chặng 9 thêm finite timeout cho IPC
> (lúc đó một task vừa có `wait_item` vừa cần `wake_tick` hợp lệ cùng lúc).
> `ntos_tick_handler()` giờ chỉ wake khi `block_reason == NTOS_BLOCK_DELAY` -
> xem §13.3 để biết `block_reason` được set/reset ở đâu.

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

> **Trạng thái thực tế (Chặng 7):** `ntos_delay()` (kernel/ntos_task.c) dùng
> critical section ngắn (`ntos_port_irq_save()`/`ntos_port_irq_restore()`) chỉ
> để ghi `wake_tick`/`state = BLOCKED` của CHÍNH task hiện tại một cách atomic,
> rồi mới `ntos_port_request_context_switch()` sau khi mở IRQ lại - tránh việc
> SysTick chen vào giữa lúc state chưa kịp chuyển BLOCKED (nếu không, PendSV sẽ
> demote nhầm task về READY thay vì giữ BLOCKED, làm mất delay). Assert cấm gọi
> từ idle task (`ntos_current_tcb != &ntos_idle_task_tcb`) và cấm gọi ngoài task
> context RUNNING. Chưa có ready list/delayed list riêng - `ntos_tick_handler()`
> (kernel/ntos_kernel.c) quét thẳng `task_table[]` mỗi tick (O(n), n ≤
> `NTOS_MAX_TASKS`). Không thêm logic so sánh priority riêng để quyết định có
> pend PendSV hay không khi wake - PendSV vẫn được request **vô điều kiện mỗi
> tick** như từ Chặng 5, và `ntos_scheduler_select_next()` (Chặng 6, không đổi)
> tự động chọn lại task priority cao nhất đang READY, nên "high wake preempt
> low" đến miễn phí từ thiết kế đã có, không cần code riêng.

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

> **Trạng thái thực tế (Chặng 7):** `ntos_tick_reached()` triển khai đúng như
> trên, đặt tại kernel/ntos_kernel.c và export qua `kernel/internal/ntos_kernel_internal.h`
> (không `static`) chỉ để test gọi trực tiếp với giá trị biên gần `UINT32_MAX`
> (xem `tests/target/test_tick_overflow.c`) - tránh phải chờ tick thật sự tràn
> số (~49 ngày ở 1 kHz). `ntos_delay()` **assert** nếu `ticks > INT32_MAX` (chữ
> ký hàm là `void`, không có chỗ trả lỗi nên chọn assert thay vì im lặng cắt bớt
> giá trị hoặc chỉ ghi tài liệu suông).

## 13.5. Delay zero

Chọn một semantics và giữ nhất quán.

Khuyến nghị:

```text
ntos_delay(0) == ntos_yield()
```

> **Trạng thái thực tế (Chặng 7):** Đã chọn đúng khuyến nghị này - `ntos_delay(0)`
> gọi thẳng `ntos_yield()` và return ngay, không set `wake_tick`/`BLOCKED`.

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

> **Trạng thái thực tế (Chặng 8 - Bước 8A):** `kernel/ntos_list.c` đã implement
> đủ 6 hàm trên đúng như thiết kế (Bước 8A, chưa tích hợp scheduler lúc đó).
> Struct `ntos_list_item_t`/`ntos_list_t` đã **chuyển từ kernel-internal sang
> public** tại `include/ntos/ntos_list.h` (chỉ struct, không có prototype hàm)
> — bắt buộc phải làm vậy vì `ntos_tcb_t` (public, `include/ntos/ntos_task.h`)
> giờ embed trực tiếp `ready_item` (Bước 8B), mà TCB không được phép include
> header kernel-internal. `kernel/internal/ntos_list_internal.h` giờ chỉ
> `#include <ntos/ntos_list.h>` rồi khai báo 6 hàm - giống hệt pattern
> `ntos_task_internal.h` đã dùng cho `ntos_task.h`. 6 hàm thao tác list **vẫn
> là kernel-internal**, không public hoá logic, chỉ public hoá layout.
>
> **Trạng thái thực tế (Chặng 8 - Bước 8B, tích hợp scheduler):**
> `ntos_tcb_t` có thêm field `ready_item` (`ntos_list_item_t`), init qua
> `ntos_list_item_init(&tcb->ready_item, tcb)` trong `ntos_task_create_static()`.
> `ready_lists[NTOS_MAX_PRIORITIES]` (kernel/ntos_kernel.c) đại diện toàn bộ
> task RUNNABLE (RUNNING + READY) theo từng priority; BLOCKED/UNUSED có
> `ready_item.container == NULL`. `ntos_scheduler_register_task()` vừa đăng ký
> `task_table[]` (vẫn giữ, chỉ dùng cho `ntos_tick_handler()` quét wake theo
> `wake_tick`, Chặng 7) vừa `ntos_list_push_back()` vào `ready_lists[priority]`.
> `ntos_delay()` gọi `ntos_list_remove(&ready_item)` khi block;
> `ntos_tick_handler()` gọi `ntos_list_push_back()` lại khi wake.
> `ntos_scheduler_select_next()` **không còn quét `task_table[]`** - quét
> `ready_lists[]` từ priority cao xuống thấp (scan tuyến tính, chưa bitmap/CLZ),
> gọi `ntos_list_next_owner()` để vừa chọn vừa tự round-robin trong đúng
> priority đó. `current_task_index` (cursor kiểu cũ) đã bị xoá vì không còn
> dùng. `ready_lists[]` được export qua `kernel_internal.h` (không `static`)
> chỉ để test kiểm tra invariant `container == &ready_lists[priority]`.

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

> **Trạng thái thực tế (Chặng 9A):** `ntos_semaphore_t` (`include/ntos/ntos_sem.h`)
> là type **công khai đầy đủ**, giống `ntos_tcb_t` - không opaque - để caller
> khai báo tĩnh trực tiếp (`static ntos_semaphore_t sem;`). Không có header
> `kernel/internal/ntos_sem_internal.h` riêng vì không module nào khác cần
> layout nội bộ của semaphore ngoài `kernel/ntos_sem.c`.

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

> **Trạng thái thực tế (Chặng 9A):** `ntos_sem_take()` (`kernel/ntos_sem.c`)
> đúng flow trên, dùng `ntos_port_irq_save()`/`ntos_port_irq_restore()`
> (PRIMASK) làm critical section, giống `ntos_delay()`. Chỉ hỗ trợ
> `NTOS_NO_WAIT` và `NTOS_WAIT_FOREVER` (§21) - **mọi giá trị timeout khác 0
> đều được xử lý như WAIT_FOREVER** (chờ vô hạn), vì Chặng 9A cố ý chưa làm
> finite timeout; không có nhánh riêng nào khác. Khi block: gọi
> `ntos_task_block_current(&sem->waiting_tasks, NTOS_BLOCK_SEMAPHORE)`
> (kernel-internal, `kernel/ntos_task.c`, dùng chung với `ntos_queue.c` - xem
> §13.2) - hàm này tự assert current đang RUNNING, không phải idle, và
> `wait_item.container == NULL` trước đó (một `wait_item` chỉ thuộc tối đa một
> wait list), đồng thời set `block_reason = NTOS_BLOCK_SEMAPHORE` để
> `ntos_tick_handler()` không wake nhầm. Dùng **direct handoff**: task được `ntos_sem_give()`/
> `give_from_isr()` wake coi như đã nhận token, `ntos_sem_take()` trả `NTOS_OK`
> ngay khi task chạy lại mà không trừ `count` lần nữa (không có nguy cơ mất
> đồng bộ vì `count` chỉ liên quan tới trường hợp KHÔNG có waiter).

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

> **Trạng thái thực tế (Chặng 9A):** `ntos_sem_give()` (`kernel/ntos_sem.c`)
> đúng flow trên. "Xác định có cần preemption không" = so sánh priority của
> waiter vừa wake với `ntos_current_tcb->priority`; chỉ `ntos_port_request_context_switch()`
> nếu waiter cao hơn - PendSV có request hay không **không đứng trong critical
> section** (request sau khi đã `ntos_port_irq_restore()`), giống nguyên tắc
> "không request context switch trước khi state/list update hoàn chỉnh".
> Không có waiter và `count == max_count` → `NTOS_ERR_RESOURCE_FULL`, không
> để `count` vượt `max_count`. Wake waiter dùng `ntos_task_make_ready()`
> (kernel-internal, `kernel/ntos_task.c`) - gỡ `wait_item`, reset
> `block_reason` về `NTOS_BLOCK_NONE`, set READY, push `ready_item` - dùng
> chung với `ntos_queue_send()`/`receive()`/`send_from_isr()` (§16.4/§16.5) vì
> đúng cùng một thao tác chuyển BLOCKED→READY, chỉ khác caller chọn waiter từ
> wait list nào.

## 15.5. Wait list ordering

Có hai lựa chọn:

1. Sort theo priority.
2. Mỗi lần give quét wait list tìm highest.

Vì project nhỏ, có thể quét để code dễ hiểu.

> **Trạng thái thực tế (Chặng 9A):** Chọn phương án 2 - `sem_select_highest_waiter()`
> (static, `kernel/ntos_sem.c`) quét tuyến tính `waiting_tasks` từ đầu tới
> cuối, so sánh priority bằng `>` (nghiêm ngặt, không phải `>=`) nên waiter
> **xuất hiện trước** trong list luôn thắng nếu cùng priority cao nhất - tức
> **FIFO giữa các waiter cùng priority**, không cần list sort hay tie-break
> riêng. `waiting_tasks` không sort theo priority - chỉ đơn thuần push_back
> theo thứ tự task gọi `ntos_sem_take()` và block.

## 15.6. ISR give

Khác normal give:

- Không block.
- Không gọi scheduler trực tiếp.
- Trả `higher_priority_task_woken`.
- Caller yêu cầu yield-from-ISR.

> **Trạng thái thực tế (Chặng 9A):** `ntos_sem_give_from_isr()` (`kernel/ntos_sem.c`)
> dùng chung logic chọn waiter/direct-handoff với `ntos_sem_give()` (qua
> helper `sem_wake_waiter()`), khác ở hai điểm: (1) luôn set
> `*higher_priority_task_woken` (`true` nếu waiter vừa wake có priority cao
> hơn `ntos_current_tcb`, ngược lại `false`) để caller/test biết được có cần
> yield không; (2) **tự gọi `ntos_port_request_context_switch()`** khi cờ này
> `true`, thay vì để caller tự request - vì ISR trong ntos chỉ có một điểm
> request context switch dùng chung (`ntos_port_request_context_switch()` chỉ
> set `SCB->ICSR` bit `PENDSVSET`, an toàn gọi từ ISR), không cần một hàm
> `portYIELD_FROM_ISR()` tách riêng như FreeRTOS. Context switch **thật sự**
> vẫn chỉ xảy ra trong `PendSV_Handler` sau khi ISR return (tail-chaining) -
> ISR không tự đổi `ntos_current_tcb`.
>
> **Không phụ thuộc thứ tự enable ngắt của BSP/application.** Waiter (nếu có)
> luôn được đưa vào `ready_lists[]` bất kể scheduler đã chạy hay chưa - đây là
> việc thuần list, không đụng `ntos_current_tcb`. Phần so priority/preempt chỉ
> chạy khi `ntos_scheduler_running == true` (cờ kernel-internal đã có sẵn từ
> Chặng 5, do `SVC_Handler` tự set ngay sau khi gán `ntos_current_tcb` hợp lệ).
> Nếu ISR give xảy ra trước đó (vd. NVIC_EnableIRQ() của một nguồn ngắt nào đó
> lỡ chạy trước `ntos_start()`), hàm chỉ đơn giản bỏ qua bước so sánh/preempt
> (`*higher_priority_task_woken` giữ `false`) thay vì dereference
> `ntos_current_tcb` lúc nó còn `NULL`. Cách này tái dùng đúng invariant kernel
> đã tự quản lý (giống `ntos_tick_handler()` dùng `ntos_scheduler_running` để
> tránh pend PendSV trước khi PSP hợp lệ) thay vì đặt ra một precondition ngầm
> định buộc BSP/application phải tự sắp thứ tự enable ngắt.

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

> **Trạng thái thực tế (Chặng 9B):** `ntos_queue_t` (`include/ntos/ntos_queue.h`)
> là type công khai đầy đủ - giống `ntos_tcb_t`/`ntos_semaphore_t`, không
> opaque - để caller khai báo tĩnh trực tiếp. Đúng layout đề xuất, không thêm
> field nào khác. Implementation tại `kernel/ntos_queue.c` (thêm vào
> `kernel/CMakeLists.txt`).

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

> **Trạng thái thực tế (Chặng 9B):** `ntos_queue_send()` (`kernel/ntos_queue.c`)
> đúng flow trên, dùng critical section PRIMASK giống `ntos_sem_take()`/
> `ntos_delay()`. **Blocking send đã implement đầy đủ** (không chỉ non-blocking) -
> `waiting_senders` hoạt động thật: `NTOS_WAIT_FOREVER` khi full gọi
> `ntos_task_block_current(&queue->waiting_senders, NTOS_BLOCK_QUEUE_SEND)`
> (kernel-internal, `kernel/ntos_task.c`, dùng chung với `ntos_sem_take()` -
> xem §13.2/§15.3) rồi request PendSV sau khi rời critical section, y hệt cách
> `ntos_queue_receive()` block vào `waiting_receivers`. Đây là quyết định
> **khác** với semaphore: semaphore không có "blocking give". Khi gửi thành
> công mà có receiver đang chờ, **không direct-handoff** như semaphore - chỉ
> wake receiver priority cao nhất bằng `ntos_task_make_ready()` (dùng chung với
> `ntos_sem_give()`, xem §15.4/§16.7), item vẫn nằm trong buffer chờ receiver
> tự copy ra.

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

> **Trạng thái thực tế (Chặng 9B):** `ntos_queue_receive()` (`kernel/ntos_queue.c`)
> đúng flow trên. Cả `send()` lẫn `receive()` dùng chung một vòng `for (;;)`
> bao quanh toàn bộ logic (copy-hoặc-block) - sau khi bị block và được wake,
> task quay lại **đầu vòng lặp** để tự kiểm tra lại điều kiện (`count > 0`/
> `count < capacity`) và tự copy, thay vì giả định thành công ngay khi resume.
> Đây là khác biệt cốt lõi so với semaphore direct-handoff (§15.3) - xem §16.7.

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

> **Trạng thái thực tế (Chặng 9B):** Đã làm cả hai mục "không bắt buộc":
> blocking send (thay vì chỉ non-blocking) vì implementation đối xứng và đơn
> giản, không tốn thêm abstraction nào so với blocking receive đã có. Không
> làm `receive_from_isr` (đúng scope §19 roadmap - ISR chỉ gửi, không có nhu
> cầu ISR nhận trong bản học tập này), không queue peek/set, không zero-copy,
> không variable-size message, không finite timeout (giống semaphore Chặng 9A,
> chỉ `NTOS_NO_WAIT`/`NTOS_WAIT_FOREVER`).

## 16.7. Important race

Sau khi một task được wake, task khác có thể chạy trước và thay đổi queue. Do đó blocking operation phải kiểm tra lại điều kiện sau khi resume.

> **Trạng thái thực tế (Chặng 9B):** Đây chính là lý do `ntos_queue_send()`/
> `ntos_queue_receive()` dùng vòng lặp `for (;;)` bao ngoài thay vì return
> ngay sau khi được wake (khác semaphore direct-handoff). Ví dụ cụ thể: hai
> receiver cùng chờ một queue capacity 1, sender gửi 1 item - receiver priority
> cao hơn được wake trước (`queue_select_highest_waiter()`, cùng thuật toán
> quét + so sánh `>` nghiêm ngặt như `sem_select_highest_waiter()`, giữ FIFO
> giữa các waiter cùng priority), nhưng nếu một task thứ ba (hoặc chính sender,
> nếu nó tiếp tục gửi rồi lại receive) chạy trước và lấy mất item trước khi
> receiver được wake kịp chạy, thì khi receiver đó thực sự resume, nó phải
> thấy `count == 0` lần nữa và tự block lại - **không được trả `NTOS_OK` với
> dữ liệu rác**. `ntos_task_make_ready()` (kernel-internal, `kernel/ntos_task.c`,
> dùng chung cho cả wake receiver/wake sender ở đây lẫn wake waiter semaphore
> §15.4) chỉ chuyển task từ wait list sang `ready_lists[priority]` và reset
> `block_reason`, không hề touch buffer hay copy item - việc copy luôn do
> chính task được wake tự làm khi nó thực sự chạy lại, ở đầu vòng `for (;;)`.

## 16.8. ISR send

> **Trạng thái thực tế (Chặng 9B):** `ntos_queue_send_from_isr()`
> (`kernel/ntos_queue.c`) không block (queue full → trả `NTOS_ERR_QUEUE_FULL`
> ngay, không bao giờ đưa ISR vào `waiting_senders` - blocking send chỉ dành
> cho task context), dùng chung `queue_select_highest_waiter()`/
> `ntos_task_make_ready()` với `ntos_queue_send()`. Giống hệt
> `ntos_sem_give_from_isr()` (Chặng 9A): chỉ so priority/set
> `*higher_priority_task_woken`/request PendSV khi `ntos_scheduler_running ==
> true`, không dereference `ntos_current_tcb` dựa trên giả định thứ tự BSP
> enable ngắt - waiter (nếu có) vẫn được wake vào `ready_lists[]` đúng đắn kể
> cả khi ISR send xảy ra trước `ntos_start()`. Không có `receive_from_isr`
> (ngoài scope Chặng 9B, xem §16.6).

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

> **Trạng thái thực tế (Chặng 6):** `ntos_idle_task()` (kernel/ntos_kernel.c, static,
> vòng lặp rỗng đúng như spec - chưa WFI) được `ntos_init()` tự tạo bằng
> `ntos_task_create_static()` với TCB/stack tĩnh (`ntos_idle_task_tcb`,
> `NTOS_IDLE_STACK_WORDS` word), priority 0. `ntos_idle_task_tcb` được export
> qua `kernel/internal/ntos_kernel_internal.h` chỉ để test so sánh con trỏ
> (`ntos_current_tcb == &ntos_idle_task_tcb`), không phải API công khai.
> Test không gọi `ntos_init()` (Chặng 3-5) sẽ không có idle - không sao vì
> chúng luôn có sẵn task application READY, không phụ thuộc idle.

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

> **Trạng thái thực tế (Chặng 10):** API công khai tại `include/ntos/ntos_stack.h`,
> implementation `kernel/ntos_stack.c` (thêm vào `kernel/CMakeLists.txt`).
> `NTOS_STACK_FILL_PATTERN` (`0xA5A5A5A5U`) thay thế literal từng có sẵn trong
> `kernel/ntos_task.c` - dùng chung một chỗ để `ntos_stack.c` và test đều tham
> chiếu đúng một pattern.
>
> `ntos_task_stack_get_info(tcb, info)`: quét `stack_low[]` từ đáy lên, đếm số
> word liên tiếp đầu tiên còn giữ đúng pattern (`untouched_words`), suy ra
> `high_water_mark_words = stack_words - untouched_words`. **Bán kính ảnh
> hưởng:** đây là mức dùng CAO NHẤT từng quan sát được (peak), KHÔNG phải mức
> đang dùng tại thời điểm gọi - một task có thể đang dùng ít hơn ngay lúc đọc vì
> các hàm sâu hơn đã return (dữ liệu cũ vẫn còn nguyên trong các word đó, không
> bị ghi lại 0xA5). O(stack_words) - **cố tình không** gọi trong tick
> handler/PendSV, chỉ dùng khi cần (debug/test), đúng yêu cầu Chặng 10.
> `guard_intact` chỉ được tính khi `NTOS_ENABLE_STACK_GUARD` bật (trước Chặng 10,
> flag này khai báo sẵn trong `ntos_config_default.h` nhưng chưa được code nào
> dùng tới - Chặng 10 là lần đầu wire nó vào logic thật); guard chính là word
> đáy cùng (`stack_low[0]`) - nếu `untouched_words == 0` nghĩa là stack đã tràn
> tới tận đáy, ghi đè cả word cảnh báo.
>
> `ntos_task_stack_guard_ok(tcb)`: kiểm tra nhanh O(1) riêng `stack_low[0]`,
> không quét toàn bộ - dùng khi chỉ cần biết "còn ổn không" mà không cần số
> liệu high-water mark đầy đủ.
>
> Assert/validate: `tcb`/`info` NULL trả `NTOS_ERR_INVALID_ARGUMENT` (API công
> khai, lỗi gọi hàm hợp lệ để trả mã lỗi thay vì assert); `tcb->state ==
> NTOS_TASK_UNUSED` (chưa từng tạo) trả `NTOS_ERR_INVALID_STATE`; bounds thật sự
> của bộ nhớ (`stack_pointer` nằm ngoài `[stack_low, stack_high)`, hoặc
> `stack_high - stack_low != stack_words`) dùng `NTOS_ASSERT` vì đó là dấu hiệu
> TCB/stack đã bị hỏng - một bug kernel thật, không phải lỗi gọi API của
> caller. `NTOS_ENABLE_STACK_FILL == 0` khiến cả hai hàm từ chối luôn (trả
> `NTOS_ERR_INVALID_STATE`/`false`) vì không có pattern nào để quét.
>
> Test: `tests/target/chang10_stack_diagnostics.c` - xác nhận task mới tạo
> (chưa từng chạy) đã báo cáo `high_water_mark_words == 16` (đúng kích thước
> initial frame dựng sẵn lúc tạo, xem §9.3), vùng dưới initial frame vẫn giữ
> nguyên pattern khi đọc thẳng mảng stack từ `main()` (trước `ntos_start()`);
> sau khi task chạy và cố ý dùng thêm một khoảng stack biết trước qua local
> array, high-water mark tăng đúng như kỳ vọng và guard vẫn nguyên (không cố
> tình overflow thật).

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

> **Trạng thái thực tế (Chặng 9A):** `NTOS_NO_WAIT`/`NTOS_WAIT_FOREVER` đã có
> sẵn tại `include/ntos/ntos_types.h` từ trước (không cần thêm). `ntos_sem_take()`
> implement đúng Giai đoạn 1 - **không** tích hợp waiter vào `wake_tick`/tick
> handler, không có race giữa timeout và event vì chưa có timeout hữu hạn nào
> để race với event cả.

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

> **Trạng thái thực tế (Chặng 10 - assert bổ sung, không đổi behavior):** Rà lại
> theo đúng bảng ở §8.3/§12.3/§12.2, `kernel/ntos_kernel.c` được bổ sung
> `NTOS_ASSERT` tại đúng các điểm chuyển state/list membership - không có nhánh
> logic nào đổi, chỉ thêm điều kiện dừng sớm nếu invariant bị vi phạm:
>
> - `ntos_scheduler_register_task()`: sau khi push task mới vào
>   `ready_lists[priority]`, assert `ready_item.container` đúng list,
>   `block_reason == NTOS_BLOCK_NONE`, `wait_item.container == NULL` - task vừa
>   vào RUNNABLE lần đầu phải sạch, không mang dấu vết BLOCKED nào.
> - `ntos_tick_handler()`: ngay trước khi đưa một task delay-expired trở lại
>   `ready_lists[]`, assert `wait_item.container == NULL` - đây là **đúng bất
>   biến mà bug lịch sử đã vi phạm** (xem ghi chú bugfix `block_reason` ở §13.2):
>   một task BLOCKED vì delay tuyệt đối không được đồng thời còn nằm trong wait
>   list của semaphore/queue nào.
> - `ntos_scheduler_select_next()`: sau khi chọn `selected`, assert thêm
>   `selected->block_reason == NTOS_BLOCK_NONE` (RUNNING/READY không được mang
>   dấu vết BLOCKED) và `stack_pointer` nằm trong `[stack_low, stack_high)` -
>   mirror đúng assert đã có sẵn ở `ntos_start()` cho first task, áp dụng lại
>   mỗi lần context switch để phát hiện sớm stack overflow/TCB hỏng.
> - `ntos_start()`: bổ sung 2 assert tương tự (`block_reason == NONE`,
>   `wait_item.container == NULL`) cho first task, để first task và mọi task
>   chọn sau đó qua `ntos_scheduler_select_next()` cùng chịu một tiêu chuẩn.
>
> Không đổi ready list architecture, không đổi PendSV/SVC, không đổi semantics
> semaphore/queue - đúng phạm vi Chặng 10 (§10 phần G AGENTS/roadmap: chỉ
> refactor/siết chặt khi có invariant đang thiếu assert, không viết lại kiến
> trúc).

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
