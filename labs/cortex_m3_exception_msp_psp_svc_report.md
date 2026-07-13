# Cortex-M3 Exception Fundamentals  
## Exception Frame, MSP, PSP, SVC và mối liên hệ với Context Switching

## Mục lục

- [1. Mục tiêu của report](#goal)
- [2. Exception trên Cortex-M3 là gì?](#what-is-exception)
- [3. Hai chế độ thực thi: Thread mode và Handler mode](#thread-handler-mode)
  - [3.1. Thread mode](#thread-mode)
  - [3.2. Handler mode](#handler-mode)
- [4. MSP và PSP](#msp-psp)
  - [4.1. Main Stack Pointer — MSP](#msp)
  - [4.2. Process Stack Pointer — PSP](#psp)
  - [4.3. Vì sao RTOS tách MSP và PSP?](#why-split-msp-psp)
- [5. Thanh ghi CONTROL](#control-register)
  - [5.1. CONTROL.SPSEL](#spsel)
  - [5.2. CONTROL.nPRIV](#npriv)
- [6. Exception entry — CPU làm gì khi exception xảy ra?](#exception-entry)
- [7. Exception frame](#exception-frame)
  - [7.1. Vì sao hardware lưu đúng các thanh ghi này?](#hw-saved-registers)
  - [7.2. Hardware-saved context và software-saved context](#hw-vs-sw-context)
- [8. Giá trị PC trong exception frame](#pc-in-frame)
- [9. EXC_RETURN](#exc-return)
  - [9.1. Vì sao `BX LR` có thể thoát exception?](#bx-lr-exception-return)
  - [9.2. Bốn bit thấp của EXC_RETURN](#exc-return-bits)
- [10. SVC là gì?](#svc-intro)
  - [10.1. SVC khác interrupt ngoại vi thế nào?](#svc-vs-interrupt)
  - [10.2. SVC dùng để làm gì?](#svc-usage)
- [11. Encoding của lệnh SVC](#svc-encoding)
- [12. Vì sao `return_address[-2]` lấy được SVC number?](#svc-number-decode)
- [13. Inline assembly wrapper cho SVC](#svc-inline-asm)
  - [13.1. Biến gắn với thanh ghi vật lý](#register-variable)
  - [13.2. Cấu trúc extended inline assembly](#extended-asm-structure)
- [14. Ý nghĩa `"+r"(arg0)`](#constraint-plus-r)
- [15. Ý nghĩa `"r"(arg1)`](#constraint-r)
- [16. Ý nghĩa `"memory"` clobber](#memory-clobber)
- [17. Vai trò của PendSV](#pendsv-role)
- [18. Vì sao không context switch trực tiếp trong ISR?](#why-not-switch-in-isr)
- [19. Các câu hỏi phỏng vấn quan trọng](#interview-questions)
- [20. Chuỗi kiến thức cần nhớ](#knowledge-chain)
- [21. Kết luận](#conclusion)

---

<a id="goal"></a>

# 1. Mục tiêu của report

Report này tập trung vào bản chất của các thành phần quan trọng trên ARM Cortex-M3:

- Exception và quá trình exception entry/return
- Exception frame
- Main Stack Pointer — MSP
- Process Stack Pointer — PSP
- Thanh ghi CONTROL
- EXC_RETURN
- Supervisor Call — SVC
- Mối liên hệ với context switching trong RTOS
- Vai trò cơ bản của PendSV

Mục tiêu chính không phải là ghi nhớ code assembly, mà là trả lời được các câu hỏi:

- Khi exception xảy ra, CPU thực sự làm gì?
- Vì sao CPU phải tự động lưu một số thanh ghi?
- Exception frame nằm ở đâu?
- MSP và PSP khác nhau như thế nào?
- Vì sao RTOS thường cho task dùng PSP và exception dùng MSP?
- SVC truyền tham số và trả kết quả như thế nào?
- Vì sao handler sửa `frame->r0` thì code trước đó nhận được kết quả qua R0?
- Vì sao phải lấy `stacked PC - 2` để đọc SVC number?
- Những phần nào được hardware lưu, phần nào kernel phải tự lưu?

---

<a id="what-is-exception"></a>

# 2. Exception trên Cortex-M3 là gì?

Exception là một sự kiện làm CPU tạm dừng luồng thực thi hiện tại để chuyển sang chạy một handler.

Exception trên Cortex-M3 bao gồm:

- Reset
- NMI
- HardFault
- MemManage
- BusFault
- UsageFault
- SVC
- PendSV
- SysTick
- Các interrupt ngoại vi

Interrupt ngoại vi cũng là một loại exception.

Có thể hiểu:

```text
Exception = khái niệm tổng quát
Interrupt = exception do ngoại vi hoặc sự kiện bất đồng bộ tạo ra
```

Khi exception xảy ra, CPU không đơn giản chỉ nhảy đến handler. Nó phải lưu đủ trạng thái để sau khi handler kết thúc, chương trình cũ có thể tiếp tục đúng vị trí.

---

<a id="thread-handler-mode"></a>

# 3. Hai chế độ thực thi: Thread mode và Handler mode

Cortex-M3 có hai execution mode chính.

<a id="thread-mode"></a>

## 3.1. Thread mode

Thread mode là chế độ chạy chương trình bình thường.

Ví dụ:

```text
main()
task A
task B
application code
```

Sau reset, CPU bắt đầu ở Thread mode.

Thread mode có thể sử dụng:

- MSP
- Hoặc PSP

Việc lựa chọn stack pointer nào được điều khiển bởi thanh ghi `CONTROL`.

<a id="handler-mode"></a>

## 3.2. Handler mode

Handler mode là chế độ CPU sử dụng khi đang chạy exception handler.

Ví dụ:

```text
SVC_Handler
PendSV_Handler
SysTick_Handler
HardFault_Handler
USART_IRQHandler
```

Quy tắc rất quan trọng:

```text
Handler mode luôn sử dụng MSP làm stack hiện hành.
```

Handler mode không dùng PSP làm stack để chạy code handler.

Tuy nhiên, exception frame của code bị ngắt có thể nằm trên MSP hoặc PSP, tùy stack mà Thread mode đang sử dụng trước khi exception xảy ra.

---

<a id="msp-psp"></a>

# 4. MSP và PSP

Cortex-M3 có hai stack pointer vật lý:

```text
MSP — Main Stack Pointer
PSP — Process Stack Pointer
```

Trong assembly có thể truy cập bằng:

```asm
mrs r0, msp
mrs r0, psp

msr msp, r0
msr psp, r0
```

<a id="msp"></a>

## 4.1. Main Stack Pointer — MSP

MSP là stack pointer mặc định sau reset.

MSP thường được dùng cho:

- Startup code
- `main()` trước khi RTOS khởi động
- Exception handler
- Interrupt service routine
- Kernel stack

Giá trị ban đầu của MSP được lấy từ word đầu tiên trong vector table.

Ví dụ vector table:

```text
0x00000000: Initial MSP value
0x00000004: Reset_Handler address
```

Khi reset:

```text
MSP <- *(vector_table + 0)
PC  <- *(vector_table + 4)
```

<a id="psp"></a>

## 4.2. Process Stack Pointer — PSP

PSP thường được dùng cho application thread hoặc task.

Trong RTOS:

```text
Task A dùng PSP trỏ vào stack A
Task B dùng PSP trỏ vào stack B
Task C dùng PSP trỏ vào stack C
```

Mỗi task có stack riêng, nhưng CPU chỉ có một thanh ghi PSP hiện hành tại một thời điểm.

TCB thường lưu giá trị PSP của task:

```c
typedef struct
{
    uint32_t *stack_pointer;
} tcb_t;
```

Khi đổi task:

```text
Lưu PSP hiện tại vào TCB task cũ
Nạp PSP từ TCB task mới
```

<a id="why-split-msp-psp"></a>

## 4.3. Vì sao RTOS tách MSP và PSP?

Thiết kế phổ biến:

```text
Thread mode / task  -> PSP
Handler mode / ISR  -> MSP
```

Lợi ích:

- Mỗi task có stack độc lập.
- Interrupt không sử dụng stack của task.
- Kernel và exception có stack riêng.
- Dễ cô lập lỗi tràn stack của task.
- Context switching chỉ cần đổi PSP.
- Có thể chạy task ở unprivileged mode trong hệ thống có bảo vệ.

Ý tưởng tổng quát:

```text
PSP chứa trạng thái của task.
MSP phục vụ kernel và exception.
```

---

<a id="control-register"></a>

# 5. Thanh ghi CONTROL

Thanh ghi `CONTROL` điều khiển một số thuộc tính của Thread mode.

Hai bit quan trọng trên Cortex-M3:

```text
CONTROL[0] = nPRIV
CONTROL[1] = SPSEL
```

<a id="spsel"></a>

## 5.1. CONTROL.SPSEL

```text
SPSEL = 0 -> Thread mode dùng MSP
SPSEL = 1 -> Thread mode dùng PSP
```

Handler mode vẫn luôn dùng MSP, bất kể `SPSEL`.

Ví dụ chuyển Thread mode sang PSP:

```asm
mrs r0, control
orr r0, r0, #2
msr control, r0
isb
```

`ISB` đảm bảo thay đổi trạng thái có hiệu lực đúng với pipeline.

<a id="npriv"></a>

## 5.2. CONTROL.nPRIV

```text
nPRIV = 0 -> Thread mode privileged
nPRIV = 1 -> Thread mode unprivileged
```

Privileged code có quyền truy cập đầy đủ hơn vào system registers và tài nguyên đặc quyền.

Một RTOS có thể cho task chạy:

```text
Thread mode
PSP
Unprivileged
```

Sau đó task dùng SVC để yêu cầu kernel thực hiện thao tác đặc quyền.

---

<a id="exception-entry"></a>

# 6. Exception entry — CPU làm gì khi exception xảy ra?

Giả sử Thread mode đang chạy một đoạn code.

Khi exception được chấp nhận, Cortex-M3 thực hiện các bước chính:

```text
1. Xác định stack đang được code bị ngắt sử dụng.
2. Tự động push exception frame lên stack đó.
3. Ghi một giá trị EXC_RETURN đặc biệt vào LR.
4. Chuyển sang Handler mode.
5. Handler mode sử dụng MSP.
6. Nạp địa chỉ handler từ vector table vào PC.
7. Bắt đầu chạy handler.
```

Ví dụ task đang dùng PSP:

```text
Task đang chạy bằng PSP
Exception xảy ra
Hardware push exception frame lên PSP
CPU vào Handler mode
Handler chạy bằng MSP
```

---

<a id="exception-frame"></a>

# 7. Exception frame

Khi exception xảy ra, hardware tự động lưu tám thanh ghi:

```text
R0
R1
R2
R3
R12
LR
PC
xPSR
```

Có thể biểu diễn bằng struct:

```c
typedef struct
{
    uint32_t r0;
    uint32_t r1;
    uint32_t r2;
    uint32_t r3;
    uint32_t r12;
    uint32_t lr;
    uint32_t pc;
    uint32_t xpsr;
} exception_frame_t;
```

Layout logic:

```text
Địa chỉ thấp
+------------------+
| R0               | <- SP sau hardware stacking
| R1               |
| R2               |
| R3               |
| R12              |
| LR               |
| PC               |
| xPSR             |
+------------------+
Địa chỉ cao
```

Stack trên Cortex-M3 là descending stack:

```text
Push -> SP giảm
Pop  -> SP tăng
```

<a id="hw-saved-registers"></a>

## 7.1. Vì sao hardware lưu đúng các thanh ghi này?

Theo ARM Procedure Call Standard, các thanh ghi:

```text
R0-R3, R12, LR
```

thuộc nhóm caller-saved hoặc scratch registers.

Hardware lưu thêm:

```text
PC
xPSR
```

để có thể tiếp tục thực thi chính xác sau exception.

Các thanh ghi:

```text
R4-R11
```

không được hardware tự động lưu.

Đây là điều cực kỳ quan trọng đối với RTOS.

<a id="hw-vs-sw-context"></a>

## 7.2. Hardware-saved context và software-saved context

Hardware tự lưu:

```text
R0-R3, R12, LR, PC, xPSR
```

RTOS thường tự lưu thêm:

```text
R4-R11
```

Context đầy đủ của task:

```text
Software-saved:
R4-R11

Hardware-saved:
R0-R3, R12, LR, PC, xPSR
```

Nếu có FPU thì Cortex-M có thể cần lưu thêm floating-point context, nhưng Cortex-M3 không có FPU tích hợp nên không cần phần đó.

---

<a id="pc-in-frame"></a>

# 8. Giá trị PC trong exception frame

`frame->pc` là địa chỉ mà CPU sẽ tiếp tục thực thi khi exception return.

Với nhiều exception, PC được lưu tương ứng với vị trí tiếp tục thực thi theo kiến trúc.

Đối với SVC:

```text
stacked PC trỏ tới lệnh ngay sau lệnh SVC.
```

Ví dụ:

```text
0x08000100: svc #1
0x08000102: instruction tiếp theo
```

Khi vào SVC handler:

```text
frame->pc = 0x08000102
```

Vì lệnh SVC là Thumb 16-bit, dài 2 byte:

```text
địa chỉ lệnh SVC = frame->pc - 2
```

Không nên khái quát rằng mọi exception đều chỉ cần `PC - 2` để tìm lệnh gây exception.

Quy tắc `PC - 2` trong report này áp dụng cụ thể cho việc tìm lại lệnh SVC 16-bit.

---

<a id="exc-return"></a>

# 9. EXC_RETURN

Khi exception xảy ra, LR trong handler không chứa return address thông thường.

Hardware ghi vào LR một giá trị đặc biệt gọi là:

```text
EXC_RETURN
```

Các giá trị thường gặp trên Cortex-M3:

```text
0xFFFFFFF1 (Handle mode + MSP)
0xFFFFFFF9 (Thread mode + MSP)
0xFFFFFFFD (Thread mode + PSP)
```

<a id="bx-lr-exception-return"></a>

## 9.1. Vì sao `BX LR` có thể thoát exception?

Trong hàm bình thường:

```asm
bx lr
```

nghĩa là branch đến địa chỉ trong LR.

Trong exception handler, nếu LR chứa mẫu EXC_RETURN hợp lệ, CPU nhận ra đây không phải địa chỉ code thông thường.

CPU thực hiện exception return:

```text
1. Chọn MSP hoặc PSP theo EXC_RETURN.
2. Pop R0-R3, R12, LR, PC, xPSR.
3. Khôi phục execution mode.
4. Tiếp tục code bị ngắt.
```

Do đó:

```asm
bx lr
```

trong handler có thể kích hoạt cả một cơ chế khôi phục context bằng hardware.

<a id="exc-return-bits"></a>

## 9.2. Bốn bit thấp của EXC_RETURN

Bốn bit thấp của `EXC_RETURN` cho biết CPU sẽ quay về mode nào và dùng stack nào để khôi phục exception frame.

```text
Bit 3  Bit 2  Bit 1  Bit 0
 Mode   Stack Reserved  1
```

* `bit 3 = 0`: return về Handler mode
* `bit 3 = 1`: return về Thread mode
* `bit 2 = 0`: restore exception frame từ MSP
* `bit 2 = 1`: restore exception frame từ PSP
* `bit 1 = 0`: reserved
* `bit 0 = 1`: giá trị EXC_RETURN hợp lệ

Các giá trị thường gặp:

```text
0xFFFFFFF1 = ...0001
-> return về Handler mode, dùng MSP

0xFFFFFFF9 = ...1001
-> return về Thread mode, dùng MSP

0xFFFFFFFD = ...1101
-> return về Thread mode, dùng PSP
```

Trong RTOS, `0xFFFFFFFD` rất phổ biến vì task thường chạy ở Thread mode bằng PSP.

Để kiểm tra exception frame nằm trên MSP hay PSP:

```asm
tst lr, #4
ite eq
mrseq r0, msp
mrsne r0, psp
```

Sau đoạn này, `R0` trỏ tới exception frame.

---

<a id="svc-intro"></a>

# 10. SVC là gì?

SVC là viết tắt của:

```text
Supervisor Call
```

SVC là một exception đồng bộ được tạo trực tiếp bởi instruction:

```asm
svc #imm8
```

Ví dụ:

```asm
svc #0
svc #1
svc #10
```

`imm8` là số 8-bit, thường dùng như system call number hoặc service number.

<a id="svc-vs-interrupt"></a>

## 10.1. SVC khác interrupt ngoại vi thế nào?

Interrupt ngoại vi xảy ra do sự kiện bên ngoài hoặc peripheral.

SVC xảy ra vì chương trình chủ động thực thi instruction:

```asm
svc #n
```

Do đó SVC là synchronous exception:

```text
Code biết chính xác instruction nào yêu cầu exception.
```

<a id="svc-usage"></a>

## 10.2. SVC dùng để làm gì?

Trong kernel hoặc RTOS, SVC thường được dùng cho:

- Gọi dịch vụ kernel
- Chuyển từ unprivileged Thread mode vào privileged Handler mode
- Khởi động task đầu tiên
- Yêu cầu cấp phát tài nguyên
- Tạo semaphore, queue hoặc task
- Thực hiện system call

Có thể hình dung:

```text
Task: "Tôi cần kernel làm một việc."
SVC: cổng chuyển có kiểm soát từ task vào kernel.
```

---

<a id="svc-encoding"></a>

# 11. Encoding của lệnh SVC

SVC là Thumb instruction 16-bit.

Encoding:

```text
15              8 7               0
+----------------+-----------------+
|     0xDF       |      imm8       |
+----------------+-----------------+
```

Ví dụ:

```asm
svc #0
```

có halfword:

```text
0xDF00
```

```asm
svc #1
```

có halfword:

```text
0xDF01
```

Cortex-M3 sử dụng little-endian, nên trong memory:

```text
svc #0 -> 00 DF
svc #1 -> 01 DF
svc #5 -> 05 DF
```

Byte ở địa chỉ thấp chứa `imm8`.

---

<a id="svc-number-decode"></a>

# 12. Vì sao `return_address[-2]` lấy được SVC number?

Code:

```c
return_address = (const uint8_t *)(uintptr_t)frame->pc;
svc_number = return_address[-2];
```

Dòng thứ hai tương đương:

```c
svc_number = *(return_address - 2);
```

Giả sử:

```text
0x08000100: svc #1
0x08000102: instruction tiếp theo
```

Trong exception frame:

```text
frame->pc = 0x08000102
```

Do SVC dài 2 byte:

```text
frame->pc - 2 = 0x08000100
```

Memory:

```text
0x08000100: 0x01
0x08000101: 0xDF
```

Vì `return_address` là `uint8_t *`, giải tham chiếu chỉ đọc một byte:

```text
return_address[-2] -> 0x01
```

Kết quả:

```text
svc_number = 1
```

---

<a id="svc-inline-asm"></a>

# 13. Inline assembly wrapper cho SVC

Ví dụ:

```c
static uint32_t svc_add(uint32_t a, uint32_t b)
{
    register uint32_t arg0 __asm("r0") = a;
    register uint32_t arg1 __asm("r1") = b;

    __asm volatile(
        "svc #1"
        : "+r"(arg0)
        : "r"(arg1)
        : "memory"
    );

    return arg0;
}
```

<a id="register-variable"></a>

## 13.1. Biến gắn với thanh ghi vật lý

```c
register uint32_t arg0 __asm("r0") = a;
```

Ý nghĩa:

```text
arg0 là tên biến C.
"r0" là tên thanh ghi vật lý.
Compiler phải đặt arg0 trong R0 tại điểm cần thiết.
```

Tương tự:

```c
register uint32_t arg1 __asm("r1") = b;
```

<a id="extended-asm-structure"></a>

## 13.2. Cấu trúc extended inline assembly

```c
__asm volatile(
    "instruction"
    : outputs
    : inputs
    : clobbers
);
```

Trong ví dụ:

```c
__asm volatile(
    "svc #1"
    : "+r"(arg0)
    : "r"(arg1)
    : "memory"
);
```

---

<a id="constraint-plus-r"></a>

# 14. Ý nghĩa `"+r"(arg0)`

Trong:

```c
"+r"(arg0)
```

`arg0` là tên biến C.

Constraint:

```text
+ = read-write operand
r = operand phải nằm trong general-purpose register
```

Nghĩa là:

```text
Trước asm:
    asm cần đọc giá trị hiện tại của arg0.

Sau asm:
    asm có thể đã thay đổi giá trị của arg0.
```

Do `arg0` được gắn với R0:

```text
Trước SVC:
    R0 = a

Sau SVC:
    R0 = kết quả
```

Compiler được thông báo rằng giá trị cũ của `arg0` không còn đáng tin sau asm và phải coi giá trị mới trong register là output.

Cách nhớ:

```text
"+r"(arg0)
= arg0 đi vào asm và cũng đi ra khỏi asm.
```

Hoặc:

```text
+ = input + output
r = register
```

Nếu chỉ dùng:

```c
"r"(arg0)
```

compiler hiểu đó chỉ là input và có thể giả định giá trị không đổi sau asm.

Nếu chỉ dùng:

```c
"=r"(arg0)
```

compiler hiểu đó chỉ là output, không cần giá trị ban đầu.

Trong trường hợp SVC add, cần cả hai:

```text
R0 ban đầu chứa a
R0 sau SVC chứa a + b
```

nên dùng `+r`.

---

<a id="constraint-r"></a>

# 15. Ý nghĩa `"r"(arg1)`

```c
"r"(arg1)
```

Đây là input-only operand.

Nghĩa là:

```text
Asm cần đọc arg1.
Compiler phải đặt arg1 trong một general-purpose register.
Asm không khai báo giá trị mới trả về qua arg1.
```

Do biến đã được gắn với R1:

```text
R1 = b
```

Handler đọc:

```c
frame->r1
```

Trong hàm `svc_add()`, không cần dùng lại `arg1` sau SVC nên input-only là phù hợp.

---

<a id="memory-clobber"></a>

# 16. Ý nghĩa `"memory"` clobber

```c
: "memory"
```

`memory` không phải tên một thanh ghi.

Nó thông báo cho compiler:

```text
Đoạn asm có thể đọc hoặc ghi memory theo cách compiler không nhìn thấy.
```

Trong trường hợp SVC:

- Handler có thể sửa global variable.
- Handler sửa exception frame.
- Handler có thể truy cập dữ liệu kernel.
- Chuỗi asm chỉ chứa `svc #1`, compiler không tự nhìn thấy code handler.

`"memory"` buộc compiler thận trọng với việc:

- Cache giá trị memory trong register.
- Di chuyển memory access qua asm.
- Tối ưu dựa trên giả định memory không thay đổi.

Cần phân biệt:

```text
"memory" clobber = compiler barrier
DMB/DSB/ISB      = instruction có tác động ở cấp CPU/kiến trúc
```

`"memory"` không tự tạo lệnh DMB, DSB hay ISB.

---

<a id="pendsv-role"></a>

# 17. Vai trò của PendSV

PendSV không có exception frame đặc biệt.

Nó sử dụng cùng cơ chế exception entry/return như các exception khác:

```text
Hardware stack exception frame
Handler mode dùng MSP
LR chứa EXC_RETURN
BX LR khôi phục frame
```

Điểm đặc biệt của PendSV nằm ở:

- Có thể được software đặt pending.
- Có thể bị trì hoãn.
- Thường được đặt priority thấp nhất.
- Chỉ chạy khi các interrupt ưu tiên cao hơn đã hoàn thành.
- Phù hợp để thực hiện context switching.

Câu cần nhớ:

```text
PendSV là một exception bình thường về cơ chế vào/ra,
nhưng được thiết kế rất phù hợp cho công việc deferred.
```

Trong RTOS:

```text
SysTick hoặc ISR:
    quyết định cần schedule
    set PendSV pending

PendSV:
    thực hiện save/switch/restore context
```

Không cần một lab riêng phức tạp về PendSV trước khi bắt đầu viết context switch thật.

---

<a id="why-not-switch-in-isr"></a>

# 18. Vì sao không context switch trực tiếp trong ISR?

Giả sử một UART interrupt đang chạy và giữa handler lại đổi task ngay lập tức.

Điều này có thể:

- Làm logic interrupt phức tạp.
- Kéo dài interrupt latency.
- Chuyển task khi ISR khác vẫn đang active.
- Gây khó kiểm soát nesting.
- Tăng nguy cơ lỗi kernel state.

Thiết kế phổ biến:

```text
ISR làm việc tối thiểu
ISR đánh dấu cần reschedule
ISR set PendSV
ISR kết thúc
PendSV priority thấp nhất mới chạy
```

Do đó context switch diễn ra tại một điểm an toàn hơn.

---

<a id="interview-questions"></a>

# 19. Các câu hỏi phỏng vấn quan trọng

## Câu 1: Cortex-M3 có bao nhiêu stack pointer?

Hai:

```text
MSP và PSP
```

## Câu 2: Handler mode dùng stack nào?

```text
Luôn dùng MSP.
```

## Câu 3: Thread mode có thể dùng stack nào?

```text
MSP hoặc PSP, tùy CONTROL.SPSEL.
```

## Câu 4: Hardware tự động lưu những register nào khi exception xảy ra?

```text
R0-R3, R12, LR, PC, xPSR
```

## Câu 5: Vì sao RTOS phải lưu R4-R11?

Vì hardware không tự lưu chúng, nhưng chúng là một phần trạng thái của task.

## Câu 6: EXC_RETURN là gì?

Là giá trị đặc biệt hardware đặt vào LR khi vào exception, mô tả cách CPU phải thoát exception.

## Câu 7: Vì sao `BX LR` trong exception handler có thể khôi phục context?

Vì LR chứa EXC_RETURN; CPU nhận diện mẫu này và tự pop exception frame.

## Câu 8: SVC dùng để làm gì?

Tạo một synchronous exception để application/task gọi dịch vụ privileged của kernel.

## Câu 9: SVC truyền tham số thế nào?

Thường truyền qua R0-R3 theo ABI. Handler đọc các giá trị này từ exception frame.

## Câu 10: SVC trả kết quả thế nào?

Handler ghi kết quả vào `frame->r0`; exception return khôi phục giá trị đó vào R0.

## Câu 11: Vì sao đọc `frame->pc - 2`?

Vì stacked PC trỏ tới instruction sau SVC, còn SVC là Thumb instruction dài 2 byte.

## Câu 12: Vì sao byte ở `PC - 2` là SVC number?

Encoding SVC là `0xDFimm8`; trên little-endian, byte thấp `imm8` nằm tại địa chỉ thấp.

## Câu 13: PendSV khác biệt quan trọng ở đâu?

Không khác về exception frame, nhưng có thể được pending và thường đặt priority thấp nhất để làm deferred context switch.

---

<a id="knowledge-chain"></a>

# 20. Chuỗi kiến thức cần nhớ

```text
Thread mode
    |
    | dùng MSP hoặc PSP
    v
Exception entry
    |
    | hardware stack R0-R3,R12,LR,PC,xPSR
    | LR = EXC_RETURN
    v
Handler mode
    |
    | luôn chạy bằng MSP
    | có thể tìm frame qua MSP hoặc PSP
    v
Handler xử lý
    |
    | có thể sửa exception frame
    v
BX LR
    |
    | exception return
    | hardware restore frame
    v
Thread mode tiếp tục
```

Với SVC:

```text
R0-R3 mang tham số
SVC imm8 chọn dịch vụ
frame->pc - 2 tìm instruction SVC
frame->r0 mang kết quả trả về
```

Với RTOS:

```text
Task dùng PSP
ISR/kernel dùng MSP
PendSV lưu R4-R11
TCB lưu PSP
Đổi PSP là trung tâm của context switch
```

---

<a id="conclusion"></a>

# 21. Kết luận

Cortex-M có hai stack pointer:

* `MSP` — Main Stack Pointer
* `PSP` — Process Stack Pointer

Khi exception xảy ra, hardware sẽ tự động lưu các thanh ghi quan trọng vào stack mà code bị ngắt đang sử dụng. Nhóm thanh ghi được lưu này gọi là **exception frame**.

Sau quá trình hardware stacking:

* Nếu code bị ngắt đang dùng `MSP`, exception frame nằm trên `MSP`.
* Nếu code bị ngắt đang dùng `PSP`, exception frame nằm trên `PSP`.

Stack pointer tương ứng sẽ trỏ tới đầu exception frame, tức vị trí chứa saved `R0`.

Khi CPU chuyển sang Handler mode, handler luôn sử dụng `MSP` để chạy, bất kể exception frame đang nằm trên `MSP` hay `PSP`.


Các khái niệm exception frame, MSP, PSP và SVC không phải những phần rời rạc.

Chúng tạo thành một cơ chế thống nhất:

```text
Exception frame
    -> lưu trạng thái Thread mode

MSP
    -> stack của handler và kernel

PSP
    -> stack riêng của task

EXC_RETURN
    -> mô tả cách khôi phục trạng thái

SVC
    -> cổng có kiểm soát từ task vào kernel

PendSV
    -> nơi phù hợp để đổi context task
```

Điểm bản chất nhất:

```text
Cortex-M3 đã cung cấp sẵn một nửa cơ chế context switching
thông qua exception entry và exception return.
```

RTOS chỉ cần bổ sung:

```text
Lưu R4-R11
Lưu PSP vào TCB
Chọn task mới
Nạp PSP mới
Khôi phục R4-R11
```

Do đó, hiểu chắc exception frame, MSP, PSP, EXC_RETURN và SVC là nền tảng trực tiếp để hiểu context switching trên Cortex-M3.
