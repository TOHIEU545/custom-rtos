# Context Switching on Cortex-M3

## Mục lục

1. [Overview](#1-overview)
2. [CPU Context](#2-cpu-context)
3. [Function Call vs Context Switch](#3-function-call-vs-context-switch)
4. [Cortex-M3 Exception Entry](#4-cortex-m3-exception-entry)
5. [Software-Saved Context](#5-software-saved-context)
6. [MSP and PSP](#6-msp-and-psp)
7. [EXC_RETURN](#7-exc_return)
8. [Why PendSV Is Used for Context Switching](#8-why-pendsv-is-used-for-context-switching)
9. [Starting the First Task with SVC](#9-starting-the-first-task-with-svc)
10. [Initial Task Stack Frame](#10-initial-task-stack-frame)
11. [Normal Context Switch Flow](#11-normal-context-switch-flow)
12. [Scheduler vs Context Switch](#12-scheduler-vs-context-switch)
13. [TCB and Stack Pointer Contract](#13-tcb-and-stack-pointer-contract)
14. [Context Switch Invariants](#14-context-switch-invariants)
15. [End-to-End Summary](#15-end-to-end-summary)

## 1. Overview

Context switch là hành động CPU dừng thực thi task A và chuyển sang thực thi
task B, gồm ba bước:

- Bảo toàn toàn bộ trạng thái (context) của task A.
- Chọn task kế tiếp cần chạy.
- Khôi phục toàn bộ trạng thái của task B.

Điều kiện tiên quyết của một context switch đúng: sau khi restore, task B
phải tiếp tục chạy đúng từ chỗ nó dừng — cùng giá trị register, cùng stack,
như thể nó chưa từng bị gián đoạn. Task B không "biết" CPU vừa chạy việc
khác trong lúc nó BLOCKED hoặc bị preempt.

Tài liệu này giải thích chính xác NTOS làm điều đó như thế nào trên
Cortex-M3: phần nào phần cứng tự lo, phần nào software (kernel + port) phải
tự tay lo, và tại sao thiết kế lại như vậy. Toàn bộ nội dung bám theo
`port/cortex_m3/ntos_port.c` và `kernel/ntos_task.c`/`ntos_kernel.c` hiện
tại.

## 2. CPU Context

Context đầy đủ của một task là toàn bộ trạng thái CPU cần thiết để tiếp tục
thực thi nó chính xác: 13 general-purpose register (R0-R12), stack pointer,
link register (LR), program counter (PC), và Program Status Register
(xPSR).

Trên Cortex-M3, khi một exception xảy ra, phần cứng tự động lưu một tập cố
định của context đó lên stack đang active — phần còn lại là trách nhiệm của
software.

| Register | Vai trò | Được lưu bởi |
| --- | --- | --- |
| R0-R3 | Argument/scratch, caller-saved theo AAPCS | Hardware (exception entry) |
| R12 | Intra-procedure-call scratch register | Hardware (exception entry) |
| LR (R14) | Địa chỉ return của caller / mang EXC_RETURN khi ở trong handler | Hardware (exception entry) |
| PC (R15) | Địa chỉ instruction đang chạy | Hardware (exception entry) |
| xPSR | Cờ trạng thái, Thumb bit, exception number | Hardware (exception entry) |
| R4-R11 | Callee-saved theo AAPCS, giữ local state sống lâu của hàm | Software (`PendSV_Handler`/`SVC_Handler`) |
| SP (PSP) | Giá trị stack pointer của task | Không nằm trong 2 frame trên — NTOS tự lưu vào `tcb->stack_pointer` |

Nhóm đầu (hardware) và nhóm `R4-R11` (software) cộng lại đúng bằng toàn bộ
context cần để resume một task — không thiếu, không thừa. Phần 4 và 5 giải
thích tại sao lại chia đúng ranh giới này chứ không phải một ranh giới khác.

## 3. Function Call vs Context Switch

Một function call bình thường (ví dụ `foo()` gọi `bar()`) và một context
switch trông có vẻ giống nhau ("nhảy sang chỗ khác rồi quay lại"), nhưng bản
chất khác nhau hoàn toàn — nhầm lẫn hai khái niệm này là nguồn gốc phổ biến
nhất của các hiểu sai về context switch.

**Function call:**

- Caller và callee nằm trong **cùng một execution context** — cùng một task,
  cùng một stack, cùng một luồng thực thi logic.
- Compiler biết trước tại compile-time rằng `bar()` sẽ được gọi, nên tự sinh
  prologue/epilogue tuân theo **AAPCS (ARM Procedure Call Standard)**: caller
  tự lưu register nó cần dùng lại sau lời gọi (caller-saved: R0-R3, R12),
  callee tự lưu/khôi phục register nó sẽ ghi đè (callee-saved: R4-R11, SP,
  LR nếu gọi hàm khác).
- `bar()` cuối cùng `return` thẳng về đúng điểm gọi trong `foo()`, bằng địa
  chỉ trong LR mà compiler đã sắp xếp.

**Context switch:**

- Task A và task B là **hai execution context hoàn toàn độc lập** — hai
  stack khác nhau, hai luồng logic không liên quan gì tới nhau.
- Task B **không phải callee của task A**. Compiler không hề biết, tại thời
  điểm task A bị ngắt, CPU sẽ chạy sang một task khác — không có lời gọi hàm
  nào giữa A và B trong source code.
- Vì không có lời gọi hàm, **không có prologue/epilogue nào được sinh ra**
  để bảo toàn context theo AAPCS. Không thể dựa vào calling convention thông
  thường để lo việc này — RTOS (ở đây là NTOS) phải **chủ động** save toàn
  bộ context của A và restore toàn bộ context của B bằng tay.

Điểm dễ hiểu sai nhất: AAPCS quy định caller-saved/callee-saved là quy ước
*giữa hai hàm gọi nhau trong cùng một luồng thực thi*. Trong context switch:

- Nhóm **caller-saved** (R0-R3, R12, cộng thêm LR/PC/xPSR để phục vụ return)
  không cần RTOS tự lưu bằng tay, vì Cortex-M3 exception mechanism **tự
  động** stack đúng nhóm này lên stack của task đang chạy khi exception xảy
  ra (phần 4) — đây là điểm hợp lý về mặt kiến trúc: tập register mà
  hardware tự lưu trùng khớp với tập caller-saved của AAPCS.
- Nhóm **callee-saved** (R4-R11) đại diện cho local state đang sống của
  task tại thời điểm nó bị ngắt — hardware không biết và không tự lưu nhóm
  này. NTOS phải tự lưu chúng trong `PendSV_Handler` (phần 5), nếu không
  task bị preempt sẽ mất giá trị local variable đang giữ trong các register
  đó.

Nói cách khác: exception hardware frame gánh đúng phần AAPCS gọi là
caller-saved (cộng thêm state cần cho việc return), còn phần callee-saved
theo AAPCS thì NTOS phải tự gánh bằng software trong PendSV.

## 4. Cortex-M3 Exception Entry

Khi bất kỳ exception nào xảy ra (SVC, PendSV, SysTick, IRQ ngoại vi...),
Cortex-M3 tự động (không cần một dòng code nào) push 8 word sau lên stack
đang active **trước khi** nhảy vào handler:

```text
high address
+-------+
| xPSR  |
| PC    |
| LR    |
| R12   |
| R3    |
| R2    |
| R1    |
| R0    |
+-------+  <- SP sau khi hardware stacking xong
low address
```

Nếu CPU đang ở Thread mode và dùng PSP (tức đang chạy một task, không phải
đang trong handler khác) thì frame này nằm trên **PSP** — chính là stack
riêng của task đang chạy. Sau khi stacking xong, CPU tự chuyển sang Handler
mode và tự chuyển sang dùng **MSP** để chạy handler (phần 6).

Đây là hành vi kiến trúc chuẩn của Cortex-M3 (ARMv7-M), không phải lựa chọn
của NTOS: mọi chương trình chạy trên lõi này đều nhận đúng 8 word này khi
exception xảy ra.

Tại sao hardware chỉ push đúng 8 register này mà không phải toàn bộ? Vì đây
là tập tối thiểu để:

- CPU có thể `bl` sang một exception handler viết bằng C bình thường (một
  hàm C bất kỳ được phép tự do dùng R0-R3/R12 mà không cần lo phá hỏng state
  của code bị ngắt), đúng như những gì một **caller-saved** register được
  phép chịu trong AAPCS.
- CPU có đủ thông tin (`PC`, `LR`, `xPSR`) để thực hiện exception return
  đúng chỗ, đúng mode, đúng cờ trạng thái.

Nói theo AAPCS: exception frame về cơ bản là "caller-saved state + return
state", được hardware tự lo thay vì compiler sinh prologue như function call
bình thường.

## 5. Software-Saved Context

Phần hardware **không** tự lưu: `R4-R11`. Theo AAPCS đây là các
callee-saved register — quy ước đó chỉ có nghĩa khi có một "callee" chủ
động lưu chúng; nhưng exception là sự kiện bất đồng bộ, không có callee
nào cả, nên **không ai** tự lưu nhóm này nếu NTOS không tự làm.

Nếu bỏ qua bước này, một task bị preempt giữa chừng (đang giữ giá trị quan
trọng trong R4-R11, ví dụ một loop counter hay con trỏ trung gian) sẽ mất
trắng các giá trị đó khi task khác chạy và ghi đè lên cùng những register
vật lý đó — task khi resume sẽ chạy tiếp với dữ liệu sai.

Vì vậy `PendSV_Handler` (`port/cortex_m3/ntos_port.c`) tự tay làm đúng việc
đó, ngay ở đầu handler — nửa đầu của hàm (nửa "save"):

```c
__attribute__((naked)) void PendSV_Handler(void)
{
    __asm volatile(
        "mrs    r0, psp\n"                                  /* Lấy PSP của task đang chạy, lúc này PSP đang trỏ vào R0 trong exception frame */
        "stmdb  r0!, {r4-r11}\n"                            /* Lưu R4-R11 đồng mỗi lần lưu giảm PSP 32 bit (4 byte) */

        "ldr    r1, =ntos_current_tcb\n"                    /* r1 = &ntos_current_tcb */
        "ldr    r2, [r1]\n"                                 /* r2 = ntos_current_tcb = địa chỉ TCB */
        "str    r0, [r2]\n"                                 /* ntos_current_tcb->stack_pointer = r0 */
        /* ... phần chọn task kế tiếp + restore, xem mục 11 ... */
    );
}
```

`stmdb r0!, {r4-r11}` là "store multiple, decrement before": ghi R4-R11 vào
4 word ngay dưới địa chỉ hiện tại của `r0` (chính là đỉnh hardware frame),
rồi tự lùi `r0` xuống đúng bằng số byte vừa ghi — sau lệnh này `r0` trỏ vào
slot `R4`, tức đỉnh của toàn bộ 16-word frame. `str r0, [r2]` ghi giá trị đó
vào `tcb->stack_pointer` — đây là bước "save" duy nhất mà PendSV thực hiện.

Kết hợp hardware frame (phần 4) và software frame (R4-R11) tạo thành đúng
toàn bộ context cần thiết để resume một task — không thiếu register nào:

```text
low address (tcb->stack_pointer trỏ vào đây sau khi save)
+-------+
| R4    |   \
| R5    |    \
| R6    |     software-saved
| R7    |     (PendSV tự lưu)
| R8    |    /
| R9    |   /
| R10   |
| R11   |
+-------+
| R0    |   \
| R1    |    \
| R2    |     \
| R3    |      hardware-saved
| R12   |     (Cortex-M3 tự lưu)
| LR    |    /
| PC    |   /
| xPSR  |  /
+-------+
high address
```

`tcb->stack_pointer` luôn trỏ vào đáy của toàn bộ frame 16 word này (slot
R4) mỗi khi task không phải RUNNING — đây chính là giá trị mà
`ntos_scheduler_select_next()`/`PendSV_Handler` dùng để restore task.

## 6. MSP and PSP

Cortex-M3 có hai stack pointer vật lý riêng biệt, chỉ một trong hai là
"active" tại một thời điểm:

- **MSP** (Main Stack Pointer) — dùng mặc định sau reset, và dùng bởi mọi
  exception/interrupt handler.
- **PSP** (Process Stack Pointer) — dùng riêng cho code chạy ở Thread mode,
  nếu được cấu hình dùng.

NTOS dùng đúng mô hình chuẩn cho RTOS trên Cortex-M3: **task chạy Thread
mode và dùng PSP; mọi exception handler (SVC, PendSV, SysTick) chạy Handler
mode và dùng MSP**. MSP được CPU tự nạp từ word đầu tiên của vector table
(`_estack`, tức đỉnh RAM — xem `bsp/stm32f10x/linker/stm32f10x.ld`) ngay lúc
reset, trước khi `main()` chạy.

Lý do thiết kế này tốt cho một RTOS:

- Stack của từng task tách hoàn toàn khỏi stack dùng chung cho kernel/ISR —
  một task tràn stack không thể ghi đè lên vùng nhớ mà SVC/PendSV/SysTick
  đang dùng để chạy.
- Mỗi task có PSP riêng, lưu trong `tcb->stack_pointer` — context switch chỉ
  cần đổi giá trị PSP, không cần đổi MSP hay di chuyển dữ liệu giữa các
  stack.
- Handler luôn có một MSP ổn định, không phụ thuộc task nào đang chạy —
  tránh việc một stack task hỏng kéo theo cả handler dùng chung stack đó bị
  hỏng theo.

NTOS không thao tác trực tiếp lên `CONTROL` register ở bất kỳ đâu trong
source. Việc CPU dùng PSP sau khi return về Thread mode được quyết định
hoàn toàn bởi giá trị `EXC_RETURN` nạp vào `LR` trước khi exception return
(phần 7): `EXC_RETURN = 0xFFFFFFFD` yêu cầu CPU return về Thread mode và
dùng PSP. NTOS chỉ cần nạp đúng giá trị này vào LR — không tự set thêm bất
kỳ register nào khác.

## 7. EXC_RETURN

Khi CPU vào một exception handler, nó không nạp vào LR một địa chỉ return
bình thường như function call — nó nạp một giá trị đặc biệt gọi là
**EXC_RETURN**. Giá trị này không phải một địa chỉ code hợp lệ; nó là một
mã lệnh cho CPU biết phải làm gì khi handler thực hiện exception return
(bằng `bx lr` với giá trị này).

`SVC_Handler` trong NTOS (`port/cortex_m3/ntos_port.c`) nạp cứng một giá trị
cụ thể trước khi return — đây là toàn bộ handler, không rút gọn:

```c
__attribute__((naked)) void SVC_Handler(void)
{
    __asm volatile("ldr   r0, =ntos_current_tcb\n"          /* r0 = &ntos_current_tcb */
                   "ldr   r0, [r0]\n"                       /* r0 = ntos_current_tcb = địa chỉ TCB */
                   "ldr   r0, [r0]\n"                       /* r0 = ntos_current_tcb->stack_pointer */
                   "ldmia r0!, {r4-r11}\n"                  /* R4-R11 được restore  */
                   "msr   psp, r0\n"                        /* PSP = dau hardware frame (slot R0) */
                   "ldr   r1, =ntos_scheduler_running\n"    /* Chi bao scheduler "running" SAU KHI PSP da hop le - */
                   "movs  r2, #1\n"                         /* neu SysTick pend PendSV som hon diem nay (vd. xen giua */
                   "strb  r2, [r1]\n"                       /* __enable_irq() va svc), PendSV_Handler van thay PSP dung. */
                   "ldr   lr, =0xFFFFFFFD\n"                /* EXC_RETURN: ve Thread mode, dung PSP */
                   "bx    lr\n"                             /* exception return - CPU tu pop R0-R3,R12,LR,PC,xPSR */
    );
}
```

Hai dòng cuối là trọng tâm của mục này: `ldr lr, =0xFFFFFFFD` nạp thẳng giá
trị EXC_RETURN vào LR (không tính toán, không đọc từ đâu khác — vì đây là
lần bootstrap duy nhất, giá trị luôn cố định), rồi `bx lr` kích hoạt exception
return với đúng giá trị đó.

`0xFFFFFFFD` là một trong ba giá trị EXC_RETURN hợp lệ trên Cortex-M3
(ARMv7-M):

| EXC_RETURN | Ý nghĩa |
| --- | --- |
| `0xFFFFFFF1` | Return về Handler mode, dùng MSP |
| `0xFFFFFFF9` | Return về Thread mode, dùng MSP |
| `0xFFFFFFFD` | Return về Thread mode, dùng PSP — giá trị NTOS dùng |

Hai bit quan trọng nhất trong các giá trị này:

- **Bit 3** — mode sẽ return về: `1` = Thread mode, `0` = Handler mode.
- **Bit 2** — stack pointer CPU sẽ dùng sau khi return: `1` = PSP, `0` =
  MSP.

`0xFFFFFFFD` có cả hai bit này bằng `1`: CPU được yêu cầu return về Thread
mode **và** dùng PSP. Đây chính là cách `SVC_Handler` "ra lệnh" cho CPU chạy
task đầu tiên ở Thread mode, dùng đúng PSP mà handler vừa set — không cần
bất kỳ lệnh nào khác thao tác lên mode hay stack pointer.

`bx lr` bên trong một exception handler không phải một branch bình thường:
CPU nhận ra giá trị trong LR là một EXC_RETURN hợp lệ (nằm trong dải giá trị
đặc biệt được kiến trúc dành riêng, không trùng với bất kỳ địa chỉ code hợp
lệ nào), và thay vì nhảy tới đó như một địa chỉ, nó kích hoạt cơ chế
exception return: pop lại 8 word hardware frame (phần 4) từ đúng stack mà
bit 2 chỉ định, khôi phục PC/xPSR/LR/R12/R0-R3, và chuyển sang mode/stack
pointer theo bit 3/bit 2.

`PendSV_Handler` **không** hardcode giá trị EXC_RETURN nào — nó `push {r3,
lr}` trước khi gọi `ntos_scheduler_select_next()` (một hàm C, sẽ ghi đè LR
qua `bl`), rồi `pop {r3, lr}` để lấy lại đúng giá trị EXC_RETURN mà CPU đã
tự nạp vào LR lúc PendSV vừa được kích hoạt. Vì PendSV luôn ngắt một task
đang chạy Thread mode/PSP, giá trị đó tự nhiên cũng là `0xFFFFFFFD` — nhưng
PendSV chỉ *bảo toàn* nó qua lệnh gọi hàm C ở giữa, không tự gán.

## 8. Why PendSV Is Used for Context Switching

Đây là một **lựa chọn thiết kế của NTOS**: PendSV là nơi duy nhất NTOS thực
hiện context switch thật. Đây không phải điều bắt buộc với mọi hệ thống
chạy Cortex-M3 — nhưng là lựa chọn được khuyến nghị rộng rãi cho RTOS trên
kiến trúc này, vì các lý do sau:

- **PendSV có thể cấu hình priority thấp nhất** trong hệ thống
  (`ntos_port_start_tick()` gọi `NVIC_SetPriority(PendSV_IRQn, ...)` với giá
  trị thấp nhất có thể). Nhờ vậy, nếu một ISR ưu tiên cao hơn (SysTick, hoặc
  IRQ ngoại vi) đang cần xử lý, PendSV luôn nhường chỗ và chỉ thật sự chạy
  khi không còn exception nào ưu tiên cao hơn đang pending — context switch
  không bao giờ chen ngang một ISR quan trọng đang dở dang.
- **Yêu cầu context switch và thực hiện context switch được NTOS tách rời**:
  bất kỳ chỗ nào cần task khác chạy (SysTick mỗi tick, `ntos_yield()`, hoặc
  `ntos_sem_give_from_isr()`/`ntos_queue_send_from_isr()` khi wake một task
  priority cao hơn) chỉ *pend* PendSV (`SCB->ICSR = SCB_ICSR_PENDSVSET_Msk`)
  — không tự thực hiện switch tại chỗ.
- Nhờ tách rời như vậy, NTOS gom toàn bộ logic save/restore context về đúng
  **một nơi duy nhất** (`PendSV_Handler`) thay vì lặp lại ở nhiều ISR khác
  nhau — đây là bất biến kiến trúc mà NTOS chủ động duy trì (xem thêm mục
  14).

Toàn bộ việc "request" đó, dù gọi từ đâu, đều chỉ là một hàm rất mỏng —
không hề đụng vào PSP hay bất kỳ register CPU nào của task:

```c
void ntos_port_request_context_switch(void)
{
    SCB->ICSR = SCB_ICSR_PENDSVSET_Msk;
    __DSB();
    __ISB();
}
```

Ghi `PENDSVSET` vào `SCB->ICSR` chỉ đặt PendSV ở trạng thái *pending* — NVIC
sẽ tự quyết định khi nào PendSV thực sự chạy, dựa trên priority hiện có.
`__DSB()`/`__ISB()` (Data/Instruction Synchronization Barrier) đảm bảo thao
tác ghi register này hoàn tất và pipeline nạp lại instruction trước khi CPU
tiếp tục, để không có khoảng trễ khó lường giữa lúc "request" và lúc NVIC
nhìn thấy PendSV đang pending.

Việc set priority thấp nhất cho PendSV cũng nằm gọn trong một hàm, chạy một
lần duy nhất trước khi scheduler bắt đầu:

```c
void ntos_port_start_tick(void)
{
    /* PendSV phải là exception ưu tiên thấp nhất - không được chen ngang bất kỳ
     * handler nào khác, context switch chỉ xảy ra khi không còn việc gì ưu tiên hơn. */
    NVIC_SetPriority(PendSV_IRQn, (uint32_t)((1UL << __NVIC_PRIO_BITS) - 1UL));

    /* SysTick_Config() tự set priority SysTick về thấp nhất (bằng PendSV) và bật
     * TICKINT + ENABLE; trả về khác 0 nếu reload value vượt quá 24-bit của SysTick. */
    NTOS_ASSERT(SysTick_Config(SystemCoreClock / NTOS_TICK_RATE_HZ) == 0U);
}
```

NTOS **không** tự save/restore context ngay trong `SysTick_Handler`.
`SysTick_Handler` chỉ gọi `ntos_tick_handler()` (kernel) — tăng tick, đánh
thức task hết delay, và — nếu cần — pend PendSV, không làm gì khác.

`ntos_port_start_tick()` (mục trên) cấu hình **SysTick và PendSV cùng một
mức priority thấp nhất**. Vì hai exception cùng priority không thể preempt
lẫn nhau, PendSV — dù đã được pend từ bên trong `SysTick_Handler` — không
thể chen vào chạy trước khi `SysTick_Handler` return; nó chỉ được CPU xử lý
ngay sau đó qua tail-chaining. Vì vậy lý do NTOS không tự switch trực tiếp
trong `SysTick_Handler` không nằm ở việc so sánh priority cao/thấp giữa hai
exception này, mà là để **gom toàn bộ logic save/restore context về đúng
một điểm duy nhất — `PendSV_Handler`** — thay vì lặp lại ở mọi ISR có thể
cần reschedule (SysTick, sem/queue `_from_isr`...).

```text
SysTick
   |
   v
ntos_tick_handler()      (kernel, portable)
   |
   v
request PendSV            (SCB->ICSR = PENDSVSET)
   |
   v
... CPU tiep tuc xu ly exception/ISR khac neu con ...
   |
   v
PendSV chay khi khong con gi uu tien hon
   |
   v
context switch that su xay ra o day
```

Các ISR khác của IPC — `ntos_sem_give_from_isr()`, `ntos_queue_send_from_isr()`
— đi theo đúng flow tương tự: nếu wake một task priority cao hơn task hiện
tại, chúng cũng chỉ request PendSV, không tự switch. Chi tiết thuật toán
wake/priority nằm ở [`ipc.md`](ipc.md), không nhắc lại ở đây.

### Why ISRs Do Not Switch Context Directly

Phần trên giải thích PendSV *được dùng như thế nào*. Phần này giải thích lý
do sâu hơn: tại sao NTOS không cho phép bất kỳ ISR nào — kể cả ISR đang
chạy ở priority cao — tự thực hiện context switch ngay tại chỗ, mà bắt buộc
phải đi qua PendSV.

Xét tình huống nesting đơn giản nhất có thể xảy ra: một task đang chạy, bị
một IRQ priority thấp ngắt, rồi chính IRQ đó lại bị một IRQ priority cao hơn
ngắt tiếp (đây là hành vi nesting chuẩn của NVIC trên Cortex-M3, không phải
điều gì đặc biệt của NTOS):

```text
Task A (Thread mode, PSP)
  |
  v
IRQ Low xay ra
  |
  v
IRQ High preempt IRQ Low
```

Diễn biến thật theo đúng cơ chế exception entry của Cortex-M3 (mục 4), từng
bước một:

1. **Task A** đang chạy Thread mode, dùng **PSP**.
2. **IRQ Low xảy ra**: stack đang active trước exception là PSP, nên hardware
   tự động push hardware frame của Task A (`R0-R3, R12, LR, PC, xPSR`) lên
   **PSP**. Sau exception entry, CPU chuyển Handler mode và IRQ Low chạy bằng
   **MSP**.
3. **IRQ High preempt IRQ Low**: lúc này CPU đang ở Handler mode, dùng MSP —
   nên stack đang active trước exception (của IRQ High) chính là MSP. Hardware
   tự động push hardware frame của IRQ Low lên **MSP**. IRQ High tiếp tục
   chạy, vẫn ở Handler mode, vẫn dùng MSP.

Trạng thái hai stack tại thời điểm IRQ High đang chạy:

```text
PSP - Task A stack
+-----------------------------+
| Task A hardware frame       |
| R0-R3, R12, LR, PC, xPSR    |
+-----------------------------+

MSP - Handler stack
+-----------------------------+
| IRQ Low hardware frame      |
| R0-R3, R12, LR, PC, xPSR    |
+-----------------------------+

CPU dang thuc thi IRQ High (Handler mode, MSP)
```

Hai frame này nằm trên hai stack khác nhau và không hề chồng lên nhau:

- **PSP** vẫn giữ nguyên hardware frame của **Task A** — không ai chạm vào
  nó trong suốt quá trình nesting này.
- **MSP** giữ hardware frame của **handler bị nested**, ở đây là IRQ Low —
  không phải của Task A.
- NVIC vẫn biết IRQ Low đang "active" nhưng tạm bị đẩy ra sau (preempted),
  chưa hề return.
- Khi IRQ High hoàn thành, CPU **phải quay lại đúng giữa IRQ Low trước**,
  theo đúng EXC_RETURN mà IRQ High nhận lúc vào — không có đường nào khác.
- Chỉ sau khi IRQ Low chạy tiếp và hoàn thành, CPU mới có thể quay về Thread
  mode (Task A), hoặc — nếu PendSV đang pending và không còn gì priority cao
  hơn — chạy PendSV trước.

Nói ngắn gọn: **task context luôn gắn với PSP, còn handler nesting luôn gắn
với MSP** — hai thứ này không bao giờ trộn lẫn trên cùng một stack.

Giả sử — chỉ để thấy vấn đề — `IRQ High` tự ý làm điều mà `PendSV_Handler`
làm: đọc PSP, restore R4-R11 của Task B, `msr psp`, nạp
`EXC_RETURN = 0xFFFFFFFD`, rồi `bx lr` để nhảy thẳng sang Task B. Việc này
sai ở nhiều lớp cùng lúc:

- **IRQ Low chưa hề hoàn thành.** Nó vẫn còn instruction chưa chạy, và
  hardware frame của nó vẫn còn nằm trên MSP, chưa được pop.
- **EXC_RETURN mà CPU tự nạp khi vào `IRQ High` không hề mang nghĩa "quay về
  Task A".** Nó mang nghĩa "quay về đúng nơi bị `IRQ High` ngắt" — tức quay
  về giữa `IRQ Low`, Handler mode, MSP. Ép `LR = 0xFFFFFFFD` rồi `bx lr` là
  tự ý thay thế giá trị đó bằng một EXC_RETURN hoàn toàn không liên quan gì
  đến ngữ cảnh nesting thật, khiến CPU "nhảy tắt" thẳng vào Thread mode/PSP
  của Task B mà bỏ hẳn quãng đường quay lại `IRQ Low` đáng lẽ phải đi qua.
- **MSP vẫn còn giữ hardware frame của `IRQ Low`** (chưa pop), và hardware
  frame gốc của **Task A trên PSP cũng bị bỏ lửng** (không task nào chủ động
  restore lại nó) — hai frame này không hề liên quan gì tới Task B, và sẽ
  không bao giờ được dọn theo flow bình thường, coi như rò rỉ stack và để
  `IRQ Low` bị bỏ dở vĩnh viễn giữa chừng.
- **NVIC vẫn coi `IRQ Low` là "active"** vì nó chưa bao giờ thực hiện đúng
  exception return của chính nó — trạng thái active/pending của NVIC bị lệch
  khỏi thực tế CPU đang chạy gì, ảnh hưởng tới toàn bộ việc xử lý priority
  sau đó.
- Nói chung, đây **không thể coi là một task-to-task switch bình thường**:
  để làm đúng, phần code thực hiện việc này sẽ phải tự biết chính xác đang ở
  nesting level nào, frame nào trên MSP thuộc về handler nào đang bị nested,
  frame nào trên PSP thực sự thuộc về task nào — tức phải tự cài đặt lại một
  phần cơ chế mà NVIC/Cortex-M3 vốn đã quản lý sẵn. Sai một bước là restore
  nhầm stack, dùng nhầm EXC_RETURN, resume nhầm mode, hoặc bỏ dở một ISR
  đang xử lý dữ liệu quan trọng (ví dụ đang thao tác dở một peripheral).

Với PendSV, NTOS tránh hoàn toàn lớp vấn đề này bằng cách **trì hoãn switch**
tới đúng thời điểm không còn nesting nào nữa:

```text
Task A
  |
  | IRQ Low
  v
IRQ Low
  |
  | IRQ High
  v
IRQ High
  |
  | wake Task B
  | request PendSV
  v
IRQ High return
  |
  v
IRQ Low tiep tuc
  |
  v
IRQ Low hoan thanh
  |
  v
PendSV chay
  |
  v
Context switch Task A -> Task B
```

`IRQ High` chỉ làm đúng phần việc thuộc về nó (wake Task B, request PendSV),
rồi return đúng theo EXC_RETURN thật của chính nó — trả CPU lại đúng giữa
`IRQ Low`, đúng frame vừa nằm trên MSP. `IRQ Low` được chạy tiếp và hoàn
thành trọn vẹn, không bị cắt ngang. PendSV, nhờ nằm ở priority thấp nhất,
chỉ được phép bắt đầu thực thi khi không còn exception nào khác đang active
hay pending ở priority cao hơn nó — tại thời điểm đó, các exception frame
của chuỗi nested handler trước đó (`IRQ Low`, `IRQ High`) đã được xử lý
xong theo đúng exception-return flow bình thường, và PSP đang thuộc về
đúng một task (Task A) — đây chính là điều kiện an toàn để `PendSV_Handler`
làm save/restore như mô tả ở mục 5 và 11.

Điều này không có nghĩa `PendSV_Handler` bất khả xâm phạm từ đầu tới cuối:
một IRQ priority cao hơn vẫn có thể preempt PendSV ngay giữa lúc nó đang
chạy, y hệt cách `IRQ High` preempt `IRQ Low` ở trên. Nếu ISR đó cũng request
reschedule, nó chỉ pend PendSV lại; sau khi ISR return, CPU quay lại tiếp
tục đúng `PendSV_Handler` đang dở, rồi mới hoàn tất switch. Priority thấp
của PendSV chỉ đảm bảo context switch không **bắt đầu** bằng cách chen ngang
một ISR priority cao hơn đang xử lý — không đảm bảo bản thân PendSV, một
khi đã bắt đầu, miễn nhiễm với mọi preemption.

Lý do sâu xa khiến việc switch giữa chừng nested interrupt phức tạp hơn
nhiều so với vẻ ngoài: task context luôn gắn với PSP, còn handler nesting
luôn gắn với MSP; EXC_RETURN mà CPU đang giữ tại bất kỳ điểm nào giữa chuỗi
nesting chỉ mô tả đúng một bước "quay lại exception liền trước" (có thể là
quay về Handler mode của một handler khác, chứ chưa chắc là Thread mode).
Muốn restore một task mới ngay giữa một chuỗi nested exception, port sẽ phải
tự suy luận đồng thời cả ba thứ: PSP/MSP đang ở trạng thái nào, nesting depth
hiện tại là bao nhiêu, và EXC_RETURN nào ứng với bước nào — thay vì để
NVIC/Cortex-M3 tự quản lý việc đó như bình thường. Trì hoãn switch tới đúng
lúc PendSV chạy (không còn nesting) loại bỏ hoàn toàn nhu cầu đó.

Đây là bất biến cốt lõi mà NTOS chủ động thiết kế:

> Một ISR có thể khiến một task khác trở thành READY, nhưng việc CPU thật
> sự chuyển sang chạy task đó chỉ xảy ra bên trong `PendSV_Handler`.

| Hành động | Nơi thực hiện |
| --- | --- |
| Wake task (đổi state BLOCKED → READY) | ISR-safe API (`_from_isr`) / kernel |
| Quyết định có cần reschedule hay không | Kernel / ISR-safe path (so priority với `ntos_current_tcb`) |
| Request context switch | `ntos_port_request_context_switch()` — chỉ pend PendSV |
| Save/restore context CPU thật | Chỉ `PendSV_Handler`, không nơi nào khác |

Đây là lựa chọn kiến trúc của NTOS, không phải điều Cortex-M3 bắt buộc —
nhưng là pattern phổ biến và phù hợp cho RTOS trên kiến trúc này, chính vì
nó loại bỏ hoàn toàn nhu cầu phải suy luận về nesting level tại nơi request
switch: bất kể request tới từ đâu, tới từ độ sâu nesting nào, PendSV luôn
là nơi duy nhất thực sự chạm vào PSP/R4-R11 của task, và nó luôn chỉ *bắt
đầu* chạy đúng lúc không còn exception nào ở priority cao hơn đang chặn nó.

## 9. Starting the First Task with SVC

Task đầu tiên không thể được khởi động bằng flow context switch bình thường
(phần 11), vì tại thời điểm đó:

- CPU chưa chạy trong context của bất kỳ task nào — nó đang chạy `main()`
  bằng MSP, ở Thread mode nhưng chưa hề dùng PSP.
- Không có "task hiện tại" nào để save — `ntos_current_tcb` trước đó là
  `NULL`.
- Không có exception frame thật nào của task để restore theo flow PendSV
  thông thường (task chưa từng bị interrupt lần nào).

Vì vậy NTOS tách bootstrap thành hai bước rõ ràng: (1) tự dựng một **initial
stack frame giả** trông giống hệt một exception frame thật (phần 10), rồi
(2) dùng **SVC** — một exception có thể được kích hoạt chủ động bằng lệnh
`svc` — để nhảy từ context "thường" của `main()` vào cơ chế exception,
nơi handler có thể tự do nạp PSP và exception-return thẳng vào task đầu
tiên.

Flow thật theo source (`kernel/ntos_kernel.c`, `port/cortex_m3/ntos_port.c`):

```text
ntos_start(first_task_tcb)
   |
   |  (kernel/ntos_kernel.c)
   |  gan ntos_current_tcb = first_task_tcb
   v
ntos_port_start_tick()        set priority PendSV/SysTick, start SysTick
   |
   v
ntos_port_start_scheduler()
   |
   |  __enable_irq()
   |  svc 0                    <- chu dong kich hoat SVC exception
   v
SVC_Handler (naked, port/cortex_m3/ntos_port.c)
   |
   |  doc ntos_current_tcb->stack_pointer
   |  ldmia r0!, {r4-r11}      restore software frame cua first task
   |  msr psp, r0              PSP tro dau hardware frame
   |  ntos_scheduler_running = true
   |  lr = 0xFFFFFFFD
   |  bx lr                    exception return
   v
CPU tu dong pop hardware frame (R0-R3, R12, LR, PC, xPSR) tu PSP
   |
   v
Task entry bat dau chay o Thread mode, dung PSP
```

`ntos_port_start_scheduler()` là toàn bộ phần "chủ động kích hoạt SVC":

```c
void ntos_port_start_scheduler(void)
{
    __enable_irq();

    __asm volatile("svc 0" ::: "memory");

    __builtin_unreachable();
}
```

Bật interrupt (`__enable_irq()`) trước khi phát `svc 0` là bắt buộc — nếu
IRQ vẫn tắt, SysTick sẽ không bao giờ chạy dù đã được cấu hình ở
`ntos_port_start_tick()`. `__builtin_unreachable()` xác nhận với compiler
rằng dòng đó không bao giờ thực thi được: `svc 0` nhảy hẳn sang
`SVC_Handler`, và handler đó exception-return thẳng vào task đầu tiên chứ
không quay lại đây. `SVC_Handler` đã được trích đầy đủ ở mục 7 — không lặp
lại ở đây.

Điểm khác biệt cốt lõi so với context switch thông thường: không có bước
"hardware save" nào ở đầu flow này (không ai bị ngắt cả) — chỉ có phần
restore. Phần "save" tương đương đã được làm sẵn một cách tĩnh, thủ công,
tại thời điểm tạo task (phần 10), chứ không xảy ra tại runtime.

## 10. Initial Task Stack Frame

Một task vừa được tạo (`ntos_task_create_static()`, `kernel/ntos_task.c`)
chưa từng chạy, nên không hề có context thật nào để restore. NTOS phải tự
dựng một **synthetic context** — một stack frame giả, đúng layout với frame
mà `SVC_Handler`/`PendSV_Handler` mong đợi khi restore — để khi được nạp,
CPU tin rằng task này vừa mới bị exception ngắt, dù thực ra nó chưa từng
chạy một instruction nào.

Frame này có đúng **16 word** (`NTOS_TASK_FRAME_WORDS = 8 word hardware + 8
word R4-R11`), được dựng từ đỉnh stack xuống:

```text
low address (stack_pointer tro vao day sau khi dung xong)
+--------+------------------------------------------+
| R4     | 0x00000004 (gia tri lap trinh, khong dung)|
| R5     | 0x00000005                                |
| R6     | 0x00000006                                |
| R7     | 0x00000007                                |
| R8     | 0x00000008                                |
| R9     | 0x00000009                                |
| R10    | 0x0000000A                                |
| R11    | 0x0000000B                                |
+--------+------------------------------------------+
| R0     | argument (tham so truyen cho task)         |
| R1     | 0x00000001 (gia tri lap trinh)             |
| R2     | 0x00000002                                 |
| R3     | 0x00000003                                 |
| R12    | 0x0000000C                                 |
| LR     | task_return_error (bay neu entry return)   |
| PC     | entry (task_entry_t)                       |
| xPSR   | 0x01000000 (bit T = 1, bat buoc Thumb)      |
+--------+------------------------------------------+
high address
```

Các thành phần thật sự có ý nghĩa (không phải giá trị lấp chỗ):

- **R0 = `argument`** — theo AAPCS, R0 là nơi tham số đầu tiên của một hàm
  được truyền vào; vì "gọi task entry" thực chất là một exception return
  nhảy thẳng vào `entry(argument)`, CPU coi R0 lúc đó chính là tham số đầu
  tiên của `entry` — đây là cách `argument` "tới tay" task mà không cần bất
  kỳ đoạn code trung gian nào truyền hộ.
- **PC = `entry`** — địa chỉ instruction đầu tiên của task sẽ chạy, được
  hardware pop vào PC ngay khi exception return.
- **LR = `task_return_error`** — nếu task entry (vốn không bao giờ được
  phép `return`) lỡ return, CPU sẽ nhảy vào một vòng lặp vô hạn thay vì
  chạy vào vùng nhớ rác.
- **xPSR = `0x01000000`** — chỉ set bit T (Thumb state, bit 24). Cortex-M3
  chỉ có Thumb state; bit này bắt buộc phải là `1`, nếu không CPU sẽ fault
  ngay khi thử thực thi instruction đầu tiên của task.
- **R1-R3, R12, R5-R11** — giá trị lấp chỗ (`0x1`..`0xB`) chỉ để dễ nhận ra
  trên debugger khi kiểm tra initial frame; không mang ý nghĩa chức năng.

Điểm bắt buộc phải đúng: `tcb->stack_pointer` sau khi dựng xong phải trỏ
đúng vào slot `R4` — đúng vị trí mà `PendSV_Handler`/`SVC_Handler` gọi
`ldmia r0!, {r4-r11}` rồi `msr psp, r0` mong đợi. **Initial stack
construction phải khớp chính xác với restore path** — nếu layout lệch dù
chỉ một word (thiếu/thừa một slot, sai thứ tự push), PendSV sẽ nạp nhầm giá
trị vào register, hoặc `msr psp` trỏ lệch vào giữa hardware frame, khiến
task chạy với register rác hoặc CPU fault ngay khi exception return.

## 11. Normal Context Switch Flow

Đây là flow xảy ra giữa hai task đã từng chạy (khác hẳn bootstrap ở phần
9-10) — ví dụ task A đang RUNNING, một sự kiện nào đó khiến cần chuyển sang
task B:

```text
Task A dang RUNNING (Thread mode, dung PSP cua A)
   |
   | exception xay ra (SysTick, hoac request PendSV tu yield/IPC)
   v
Hardware tu dong stack: R0-R3, R12, LR, PC, xPSR   (len PSP cua A)
   |
   v
PendSV_Handler bat dau chay (Handler mode, MSP)
   |
   | mrs r0, psp                 r0 = dinh hardware frame cua A
   | stmdb r0!, {r4-r11}         software save: day R4-R11 xuong duoi
   | ntos_current_tcb->stack_pointer = r0   (luu vao TCB cua A)
   |
   | bl ntos_scheduler_select_next()   (kernel, C - chon TCB tiep theo)
   |
   | r0 = TCB_B->stack_pointer
   | ldmia r0!, {r4-r11}         software restore: nap R4-R11 cua B
   | msr psp, r0                 PSP = dinh hardware frame cua B
   v
bx lr (EXC_RETURN da duoc bao toan qua push/pop {r3,lr})
   |
   | hardware tu dong pop: R0-R3, R12, LR, PC, xPSR  (tu PSP cua B)
   v
Task B tiep tuc chay (Thread mode, dung PSP cua B)
```

Toàn bộ flow trên ứng đúng với `PendSV_Handler` thật, không rút gọn —
nửa đầu là "save" (đã trích ở mục 5), nửa sau là "chọn task + restore":

```c
__attribute__((naked)) void PendSV_Handler(void)
{
    __asm volatile(
        "mrs    r0, psp\n"                                  /* Lấy PSP của task đang chạy, lúc này PSP đang trỏ vào R0 trong exception frame */
        "stmdb  r0!, {r4-r11}\n"                            /* Lưu R4-R11 đồng mỗi lần lưu giảm PSP 32 bit (4 byte) */

        "ldr    r1, =ntos_current_tcb\n"                    /* r1 = &ntos_current_tcb */
        "ldr    r2, [r1]\n"                                 /* r2 = ntos_current_tcb = địa chỉ TCB */
        "str    r0, [r2]\n"                                 /* ntos_current_tcb->stack_pointer = r0 */

        "push   {r3, lr}\n"                                 /* Lưu EXC_RETURN trong lr và giữ MSP align 8 byte trước khi goin hàm C */
        "bl     ntos_scheduler_select_next\n"               /* Gọi scheduler C để chon task tiếp theo; BL tạm ghi đè lr */
        "pop    {r3, lr}\n"                                 /* Khôi phục EXC_RETURN ban đầu vao lr sau khi scheduler return */

        "ldr    r1, =ntos_current_tcb\n"                    /* r1 = &ntos_current_tcb (ntos_current_tcb có thể đã đổi sau hàm scheduler) */
        "ldr    r2, [r1]\n"                                 /* r2 = ntos_current_tcb = địa chỉ TCB */
        "ldr    r0, [r2]\n"                                 /* r0 = ntos_current_tcb->stack_pointer */

        "ldmia  r0!, {r4-r11}\n"                            /* R4-R11 được restore  */
        "msr    psp, r0\n"                                  /* PSP = dau hardware frame (slot R0) */
        "bx     lr\n"                                       /* exception return - CPU tu pop R0-R3,R12,LR,PC,xPSR */
    );
}
```

`push {r3, lr}` / `pop {r3, lr}` là chi tiết dễ bị bỏ sót nhưng bắt buộc:
`bl ntos_scheduler_select_next` là một lời gọi hàm C thật sự, và theo AAPCS,
`bl` sẽ ghi địa chỉ return vào `LR` — tức ghi đè đúng giá trị EXC_RETURN mà
`PendSV_Handler` cần giữ lại để `bx lr` ở cuối hàm hoạt động đúng. Vì vậy LR
(cùng `r3` để giữ stack 8-byte-aligned trước khi gọi hàm C, theo AAPCS) phải
được cất tạm trên MSP (`push`), rồi lấy lại (`pop`) ngay sau khi
`ntos_scheduler_select_next()` return.

Hai giai đoạn tách bạch rõ ràng, không được nhầm lẫn:

- **Hardware save/restore** — Cortex-M3 tự làm, xảy ra ở lúc exception entry
  (save) và lúc `bx lr` với EXC_RETURN hợp lệ (restore). NTOS không viết
  code cho phần này.
- **Software save/restore** — chỉ đúng 2 lệnh `stmdb`/`ldmia` cho R4-R11
  mỗi chiều, do chính `PendSV_Handler` viết tay. `PendSV_Handler` **không**
  tự lưu toàn bộ register — nó chỉ lo đúng phần mà hardware không lo
  (phần 5).

## 12. Scheduler vs Context Switch

Hai trách nhiệm này tách biệt hoàn toàn trong NTOS:

- **Scheduler** — quyết định *task nào* sẽ chạy tiếp theo. Đây thuần là một
  bài toán chọn lựa dựa trên priority/ready list (`ntos_scheduler_select_next()`,
  `kernel/ntos_kernel.c`), không đụng tới bất kỳ register CPU nào.
- **Context switch** — thực sự save CPU state hiện tại và restore CPU state
  của task được chọn (`PendSV_Handler`, `port/cortex_m3/ntos_port.c`).

`PendSV_Handler` gọi `ntos_scheduler_select_next()` ở giữa — sau khi save
xong context của task cũ, trước khi restore context của task mới — nhưng
bản thân hàm scheduler chỉ đọc/ghi `ntos_current_tcb` và các list nội bộ,
hoàn toàn không biết PSP, R4-R11, hay EXC_RETURN là gì.

Tách như vậy giúp `kernel/` giữ đúng nguyên tắc CPU-độc lập: toàn bộ kiến
thức về "Cortex-M3 context switch làm việc thế nào" nằm gọn trong
`port/cortex_m3/`, kernel chỉ cần gọi ra ngoài qua
`ntos_port_request_context_switch()` mà không cần biết PendSV/PSP/EXC_RETURN
tồn tại.

## 13. TCB and Stack Pointer Contract

Để context switch hoạt động, kernel và port chỉ cần thống nhất: TCB (`struct ntos_tcb`,
`include/ntos/ntos_task.h`) phải lưu `stack_pointer` của task, và vị trí
(offset) của field đó trong struct phải cố định, port biết trước để đọc/ghi
trực tiếp bằng assembly — không qua một accessor C nào, vì
`PendSV_Handler`/`SVC_Handler` là hàm `naked`, không có function-call
overhead để gọi getter/setter. Ngoài đúng field này, port không cần (và
không được) biết gì thêm về phần còn lại của TCB.

`port/cortex_m3/ntos_port.c` khẳng định điều này bằng compile-time
check:

```c
_Static_assert(offsetof(struct ntos_tcb, stack_pointer) == 0U,
               "stack_pointer phai la field dau tien trong struct ntos_tcb");
```

`stack_pointer` phải là field **đầu tiên** vì đoạn assembly trong
`SVC_Handler`/`PendSV_Handler` đọc/ghi nó bằng `ldr r0, [r2]` /
`str r0, [r2]` với `r2` là địa chỉ TCB — tức offset `0`. Nếu ai đó thêm một
field mới lên trước `stack_pointer` trong tương lai mà quên cập nhật phần
assembly, `_Static_assert` này sẽ chặn build ngay tại compile-time thay vì
để lỗi rơi xuống runtime (nạp sai offset → restore sai register → fault khó
debug).

Đây là ranh giới chặt nhất giữa hai module: port được phép "nhìn xuyên" vào
đúng field này của TCB qua `kernel/internal/`, nhưng không có quyền gì khác
với phần còn lại của TCB hay với thiết kế task nói chung.

## 14. Context Switch Invariants

| Invariant | Lý do |
| --- | --- |
| Mỗi task có PSP/stack riêng | Cô lập lỗi giữa các task; context switch chỉ cần đổi giá trị PSP, không di chuyển dữ liệu |
| `stack_pointer` luôn 8-byte aligned | Yêu cầu AAPCS cho stack tại điểm gọi hàm ngoài (function call boundary); Cortex-M3 exception entry cũng tự align 8 byte |
| `stack_pointer` là field đầu tiên trong `ntos_tcb_t` | Assembly trong `SVC_Handler`/`PendSV_Handler` đọc/ghi trực tiếp qua offset 0, được khẳng định bằng `_Static_assert` |
| Initial stack frame phải khớp chính xác layout mà restore path mong đợi | Sai một word → nạp nhầm register hoặc PSP trỏ lệch, dẫn tới chạy sai hoặc fault ngay khi exception return |
| PendSV là nơi duy nhất NTOS thực hiện context switch thật | Lựa chọn thiết kế của NTOS (mục 8) — mọi nơi khác chỉ được *request*, không tự switch |
| Handler (SVC/PendSV/SysTick) luôn dùng MSP | Cô lập stack hệ thống khỏi stack của task đang chạy |
| Task luôn dùng PSP (từ sau khi first task start) | Đảm bảo context switch chỉ cần thao tác trên PSP, không đụng MSP |
| Software frame (R4-R11) + hardware frame (R0-R3,R12,LR,PC,xPSR) phải luôn đi cùng nhau | Hai phần này cộng lại mới là context đầy đủ; thiếu một phần task sẽ resume với state sai |

## 15. End-to-End Summary

**Task mới (chưa từng chạy):**

```text
ntos_task_create_static()
   -> dung synthetic 16-word frame (muc 10)
   -> (chi voi first task) SVC
   -> SVC_Handler restore frame, msr psp, bx lr
   -> task entry bat dau chay
```

**Task đã chạy, bị context switch (mọi lần sau đó):**

```text
exception xay ra (SysTick / yield / IPC wake)
   -> hardware save (R0-R3, R12, LR, PC, xPSR len PSP)
   -> PendSV_Handler software save (R4-R11)
   -> ntos_scheduler_select_next() chon TCB tiep theo
   -> PendSV_Handler software restore (R4-R11 cua TCB moi)
   -> hardware restore (bx lr, EXC_RETURN = Thread mode + PSP)
   -> task tiep tuc chay dung tu diem no bi ngat
```

Hai flow này chỉ khác nhau ở đúng một chỗ: task mới không có gì để hardware
save (vì chưa từng chạy), nên phần "save" được làm sẵn một cách tĩnh, thủ
công, tại thời điểm tạo task — thay vì xảy ra tại runtime như mọi context
switch bình thường.
