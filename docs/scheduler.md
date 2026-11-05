# MyRTOS Scheduler

## 1. Overview

Scheduler trả lời đúng một câu hỏi:

> Trong số các task đang READY, task nào được quyền chạy tiếp theo?

Scheduler **không phải** context switch. Nó không đụng tới PSP, không lưu
hay khôi phục register nào — nó chỉ đọc/ghi state của TCB và cấu trúc ready
list để quyết định *task tiếp theo là ai*. Việc thực sự chuyển CPU sang
chạy task đó (lưu context task cũ, khôi phục context task mới) là việc của
`PendSV_Handler` ở `port/ARM/cortex-m3/` — xem chi tiết ở
[`context_switch.md`](context_switch.md).

```text
Event (tick / yield / IPC wake)
  |
  v
Kernel state thay doi (tcb->state, ready_lists[])
  |
  v
Scheduler chon next task (rtos_scheduler_select_next())
  |
  v
PendSV thuc hien context switch that (port/ARM/cortex-m3/)
```

Tài liệu này chỉ nói về hai bước đầu — quyết định *ai chạy tiếp theo* và
*khi nào cần hỏi lại quyết định đó* — dựa trên `kernel/src/core.c`,
`kernel/src/task.c`, `kernel/src/core.c` hiện tại.

## 2. Scheduling Policy and Priority Model

MyRTOS dùng **fixed-priority scheduling**: mỗi task có một priority cố định
(gán lúc tạo, không đổi trong suốt vòng đời), và tại mọi thời điểm cần chọn
task, scheduler luôn ưu tiên tuyệt đối task READY có priority cao nhất hiện
có — không có time-slice chia sẻ giữa các priority khác nhau như một số
scheduler khác (ví dụ Completely Fair Scheduler).

| | Giá trị thật |
| --- | --- |
| Kiểu dữ liệu priority | `rtos_priority_t` = `uint8_t` (`kernel/include/rtos.h`) |
| Chiều ưu tiên | Số **lớn hơn** = ưu tiên **cao hơn** |
| Range hợp lệ | `0 .. RTOS_MAX_PRIORITIES - 1`, với `RTOS_MAX_PRIORITIES = 9` (`rtos_config_default.h`) |
| Priority thấp nhất (`0`) | Idle task — tự tạo trong `rtos_init()`, không dùng cho task application |

`rtos_task_create_static()` từ chối thẳng priority `>= RTOS_MAX_PRIORITIES`
(`RTOS_ERR_INVALID_PRIORITY`) — không có priority "ngoài range" nào tồn tại
trong hệ thống.

Vì sao luôn chọn priority cao nhất trước, không chia sẻ CPU công bằng giữa
các priority: đây là đánh đổi cố ý của fixed-priority — task priority thấp
có thể bị **starvation** hoàn toàn nếu luôn có task priority cao hơn ở
trạng thái READY (kể cả idle: nó chỉ chạy khi *không còn* task application
nào READY). Đổi lại, hành vi hoàn toàn có thể dự đoán được: task priority
cao nhất đang READY *luôn* là task chạy tiếp theo, không phụ thuộc lịch sử
chạy trước đó — tính chất quan trọng cho real-time.

MyRTOS hiện tại **không có** priority inheritance, priority ceiling, hay
dynamic priority — nếu cần các cơ chế đó, đây là giới hạn thiết kế hiện tại
(xem [`known_limitations.md`](known_limitations.md)), không phải điều tài
liệu này giả định sẽ có.

## 3. Ready Lists

Ready list là cấu trúc lưu **mọi task có thể chạy được** (RUNNABLE = RUNNING
hoặc READY) tại một priority, để chọn task tiếp theo là một phép tra cứu
theo priority thay vì quét toàn bộ `RTOS_MAX_TASKS` task mỗi lần cần chọn.

```c
rtos_list_t ready_lists[RTOS_MAX_PRIORITIES];
```

Mỗi phần tử là một `rtos_list_t` — intrusive doubly-linked circular list
dùng sentinel (`kernel/src/core.c`, xem `kernel/include/rtos.h`). "Intrusive" nghĩa
là node của list (`rtos_list_item_t`) nằm ngay bên trong struct sở hữu nó,
không cấp phát riêng:

```c
typedef struct rtos_tcb
{
    ...
    rtos_list_item_t ready_item;  /* node trong ready_lists[priority] */
    rtos_list_item_t wait_item;   /* node trong wait list cua sem/queue */
} rtos_tcb_t;
```

`ready_item.container == &ready_lists[tcb->priority]` khi và chỉ khi task
đang RUNNABLE (RUNNING hoặc READY). Khi task chuyển BLOCKED,
`ready_item` bị `rtos_list_remove()` khỏi list — `container` trở về `NULL`.

Điểm dễ hiểu nhầm nhất: **task RUNNING không bị lấy ra khỏi ready list**.
Nó vẫn nằm nguyên trong `ready_lists[priority]` của nó; chỉ có `tcb->state`
là `RTOS_TASK_RUNNING` thay vì `RTOS_TASK_READY`. Lý do: khi
`rtos_scheduler_select_next()` chạy, nó không cần biết "ai đang chạy" bằng
một cơ chế riêng — nó chỉ demote RUNNING hiện tại về READY (đổi state tại
chỗ, không đụng list) rồi quét lại toàn bộ ready list y hệt như chọn lần
đầu (mục 4).

```text
ready_lists[7] (cao nhat) -> (rong)
ready_lists[6]            -> (rong)
ready_lists[5]            -> Task_H
ready_lists[4]            -> (rong)
ready_lists[3]            -> Task_M1 -> Task_M2
ready_lists[2]            -> (rong)
ready_lists[1]            -> (rong)
ready_lists[0] (thap nhat)-> Idle
```

Mỗi `rtos_list_t` tự giữ một cursor round-robin riêng (`index`), độc lập
với các priority khác — round-robin chỉ có ý nghĩa *giữa các task cùng
priority*, không lan sang priority khác (mục 5). Task được wake (hết delay,
hoặc IPC give/send) chỉ đơn giản `rtos_list_push_back()` trở lại đúng
`ready_lists[priority]` của nó — không có danh sách "delayed" hay "waiting"
riêng ở tầng scheduler; `task_table[]` (mảng phẳng mọi task, không phải
theo priority) chỉ tồn tại để `rtos_tick_handler()` quét tìm task hết
delay, không dùng để chọn task chạy.

### Why a Doubly-Linked Circular List?

Bốn lựa chọn cấu trúc dữ liệu ở trên không phải ngẫu nhiên — mỗi đặc điểm
giải quyết đúng một vấn đề mà một RTOS gặp thường xuyên.

**Doubly-linked.** Task đổi state rất thường xuyên (READY → BLOCKED khi
delay/chờ IPC, rồi BLOCKED → READY khi wake) — mỗi lần như vậy là một lần
remove hoặc insert vào giữa ready list/wait list, không phải chỉ ở đầu hay
cuối. Vì mỗi `rtos_list_item_t` có cả `next` lẫn `previous`, `rtos_list_remove()`
chỉ cần nối lại hai hàng xóm của node đã biết địa chỉ:

```text
A <-> B <-> C        remove B        A <-------> C
```

Vì caller luôn có sẵn con trỏ tới đúng node cần gỡ (`&tcb->ready_item`),
không cần dò từ đầu list để tìm "node đứng trước B" như singly-linked list
sẽ buộc phải làm — `rtos_list_remove(item)` là O(1) theo đúng nghĩa: chi phí
không phụ thuộc list dài hay ngắn. (Claim O(1) này chỉ áp dụng cho
`rtos_list_remove()`/`rtos_list_push_back()`/`rtos_list_next_owner()` —
không suy rộng sang thao tác khác ngoài `kernel/src/list.c`.)

**Circular.** Mục đích chính không phải để biết "đầu"/"cuối" danh sách —
round-robin (mục 5) chỉ cần liên tục đi `next` từ một task sang task kế
tiếp, mãi mãi:

```text
A -> B -> C
^         |
|_________|
```

`rtos_list_next_owner()` tiến cursor tới `next` vô điều kiện; nếu chạm sentinel
thì tự động vòng qua nó để tới phần tử đầu — không cần một nhánh `if (current
== last) current = first` riêng. Circular hoá đúng chỗ round-robin thật sự
cần: "sau task cuối cùng luôn là task đầu tiên", không phải một tiện ích phụ.

**Sentinel.** `rtos_list_t` không dùng `NULL` để đánh dấu đầu/cuối — nó có
một node mốc cố định tên `end` (`kernel/include/rtos.h`), không đại diện task nào.
`list->end.next` trỏ tới phần tử đầu, `list->end.previous` trỏ tới phần tử
cuối; list rỗng là khi `end` tự trỏ vào chính nó:

```text
Rong:                    Co du lieu:
   ,--.                     ,--.
   v  |                     v  |
  [end]                   [end]<->A<->B<->C
   ^  |                          ^        |
   `--'                          `--------'
```

Nhờ vậy `rtos_list_push_back()`/`rtos_list_remove()` dùng **chung một logic**
cho mọi vị trí — chèn/gỡ ở đầu, cuối, hay giữa danh sách đều là "nối lại hai
node lân cận", không có nhánh riêng cho "list đang rỗng" hay "gỡ phần tử
cuối cùng". Đổi lại việc phải luôn nhớ *không được* coi `end` là một item
thật (`rtos_list_next_owner()`/`rtos_list_remove()` đều assert điều này).

**Intrusive.** Node list (`rtos_list_item_t`) nằm ngay bên trong `rtos_tcb_t` — phù hợp trực tiếp với
nguyên tắc static allocation của MyRTOS (không heap, không cấp phát node lúc
runtime). Nhờ node nằm sẵn trong TCB, mỗi task có hai node độc lập cho hai
vai trò khác nhau: `ready_item` cho `ready_lists[priority]`, và `wait_item`
cho wait list của semaphore/queue — tách riêng để mỗi vai trò có liên kết
list của chính nó, không bao giờ dùng chung một node cho hai list khác
nhau:

```text
RUNNABLE (READY/RUNNING)          BLOCKED vi IPC
  ready_item -> ready list           ready_item -> khong gan list nao
  wait_item  -> khong gan list nao   wait_item  -> wait list cua sem/queue
```

Tại mọi thời điểm, đúng một trong hai node đang gắn vào một list — không
bao giờ cả hai cùng lúc: `rtos_task_block_current()` gỡ `ready_item` ra
khỏi ready list rồi mới chèn `wait_item` vào wait list; `rtos_task_make_ready()`
làm ngược lại. Đây là bất biến được kiểm tra bằng `RTOS_ASSERT` ở cả hai
hàm (`kernel/src/task.c`).

| Design | Why |
| --- | --- |
| Intrusive | Không cần cấp phát node riêng — khớp static allocation. Nếu không thì sẽ cần 1 vùng nhớ riêng cho các node và TCB sẽ giữ con trỏ trỏ tới node đó |
| Doubly-linked | Remove một node đã biết địa chỉ trong O(1) |
| Circular | Round-robin tự quay lại đầu, không cần nhánh đặc biệt |
| Sentinel (`end`) | Một logic insert/remove duy nhất cho mọi vị trí, kể cả list rỗng |

## 4. Scheduler Selection Algorithm

Toàn bộ thuật toán chọn task nằm trong `rtos_scheduler_select_next()`
(`kernel/src/core.c`), được gọi từ `PendSV_Handler` sau khi context của
task cũ đã được lưu:

```c
void rtos_scheduler_select_next(void)
{
    int p;
    rtos_tcb_t *selected;

    if (rtos_current_tcb->state == RTOS_TASK_RUNNING)
    {
        rtos_current_tcb->state = RTOS_TASK_READY;
    }

    selected = NULL;
    for (p = (int)RTOS_MAX_PRIORITIES - 1; p >= 0; --p)
    {
        if (!rtos_list_is_empty(&ready_lists[p]))
        {
            selected = (rtos_tcb_t *)rtos_list_next_owner(&ready_lists[p]);
            break;
        }
    }

    rtos_current_tcb = selected;
    rtos_current_tcb->state = RTOS_TASK_RUNNING;
}
```

```text
Current task (neu dang RUNNING)
   |
   v
Demote ve READY (khong doi list, task van nam trong ready_lists[])
   |
   v
Quet ready_lists[] tu priority cao nhat xuong thap nhat
   |
   v
Gap list khong rong dau tien -> tien round-robin cursor, lay owner
   |
   v
rtos_current_tcb = selected
   |
   v
Danh dau selected la RUNNING
```

Vài điểm bản chất:

- **Không có bước "so sánh" giữa task cũ và task mới.** Scheduler không hỏi
  "task mới có ưu tiên cao hơn task cũ không" — nó chỉ đơn giản quét lại từ
  đầu mỗi lần được gọi. Vì task RUNNING vẫn nằm trong ready list (mục 3),
  quét lại luôn tự nhiên chọn đúng: nếu không có gì thay đổi, nó sẽ chọn
  lại đúng task đang chạy (round-robin cursor của list đó tiến sang phần
  tử kế, có thể là chính nó nếu list chỉ có một task).
- **Idle task đảm bảo `ready_lists[0]` không bao giờ rỗng** — idle không
  bao giờ block, không bao giờ bị remove khỏi ready list, nên vòng lặp quét
  luôn tìm được ít nhất một task (mục 10).
- Hàm này **không** lưu/khôi phục bất kỳ register CPU nào, không đụng PSP —
  nó chỉ thao tác trên `rtos_current_tcb`, `tcb->state`, và cấu trúc
  `ready_lists[]`. Save/restore context là việc của `PendSV_Handler` gọi
  hàm này ở giữa (xem [`context_switch.md`](context_switch.md) mục 11).

## 5. Round-Robin and Yield

### Round-robin

Fixed-priority quyết định **priority group nào** được chạy; round-robin
quyết định **task nào trong cùng group** được chạy ở mỗi lần chọn. Cơ chế
này hoàn toàn nằm trong cursor `index` riêng của từng `rtos_list_t`
(`kernel/src/core.c`):

```c
void *rtos_list_next_owner(rtos_list_t *list)
{
    list->index = list->index->next;
    if (list->index == &list->end)
    {
        list->index = list->index->next;  /* bo qua sentinel, vong lai dau */
    }
    return list->index->owner;
}
```

Với ba task A, B, C cùng priority 2, mỗi lần `ready_lists[2]` được chọn:

```text
Priority 2: A -> B -> C (vong tron)

Lan chon #1 -> A
Lan chon #2 -> B
Lan chon #3 -> C
Lan chon #4 -> A
```

Cursor không reset mỗi lần — nó nhớ vị trí đã duyệt tới, nên round-robin
diễn ra tuần tự qua nhiều lần `select_next()` liên tiếp, không lặp lại từ
đầu list mỗi lần.

### Yield

`rtos_yield()` (`kernel/src/task.c`) là toàn bộ:

```c
void rtos_yield(void)
{
    rtos_port_request_context_switch();
}
```

Chỉ đúng một dòng — **`rtos_yield()` không tự đổi state của task gọi nó**.
Task vẫn `RUNNING` cho tới khi `PendSV_Handler` thực sự chạy; lúc đó
`rtos_scheduler_select_next()` mới demote nó về `READY` (như mọi task
RUNNING khác bị chọn lại, mục 4) rồi chọn tiếp theo round-robin. Nói cách
khác, yield không phải một API "block" — nó chỉ là một cách *yêu cầu* hỏi
lại scheduler ngay bây giờ thay vì đợi tick kế tiếp.

Nếu task gọi yield là task **duy nhất** ở priority cao nhất đang READY:
round-robin cursor của list đó (chỉ có một phần tử) sẽ tiến rồi vòng lại
đúng phần tử đó — `rtos_scheduler_select_next()` chọn lại chính task vừa
yield. PendSV vẫn chạy trọn vẹn (save + restore context), nhưng kết quả
quan sát được là task tiếp tục chạy như không có gì xảy ra.

Phân biệt rõ **yield** và **block**: yield không rời khỏi ready list, task
vẫn RUNNABLE trong suốt quá trình; block (`rtos_delay()`,
`rtos_task_block_current()` — mục 7, 8) chủ động rời khỏi ready list và chỉ
quay lại khi có ai đó (tick handler hoặc IPC) đưa nó về READY.

## 6. Preemptive Scheduling

Preemption nghĩa là một task RUNNING có thể mất CPU **mà không tự nguyện
gọi gì cả** — khác hẳn yield, nơi task chủ động nhường CPU.

MyRTOS là preemptive: mọi event làm một task khác trở thành READY có priority
cao hơn task đang RUNNING đều dẫn tới context switch, dù task đang chạy
không hề gọi bất kỳ API nào. Ba nguồn preemption thật trong source:

- **Tick** (mục 7) — mỗi tick, nếu có task hết delay, nó được đưa về READY;
  `rtos_tick_handler()` request PendSV vô điều kiện.
- **IPC wake** (mục 8) — `rtos_sem_give()`/`rtos_queue_send()`/`receive()`
  (và biến thể `_from_isr`) có thể đưa một task priority cao hơn về READY
  và request PendSV.
- **ISR wake task priority cao hơn** — cùng cơ chế IPC wake, nhưng xảy ra
  từ ngữ cảnh ISR thay vì task.

```text
Task Low dang RUNNING
   |
   v
Event (tick/IPC) danh thuc Task High
   |
   v
Task High -> READY (van nam trong ready_lists[])
   |
   v
Request PendSV
   |
   v
rtos_scheduler_select_next() chon Task High (priority cao hon)
   |
   v
Context switch (PendSV_Handler)
```

Tại sao task priority cao hơn luôn thắng: `rtos_scheduler_select_next()`
quét `ready_lists[]` từ priority cao nhất xuống (mục 4) — nó không biết và
không cần biết "ai vừa được wake"; nó chỉ đơn giản luôn tìm thấy priority
cao nhất đang có task READY trước tiên. Đây là lý do các nơi wake task chỉ
cần request PendSV (đặt lại câu hỏi "ai chạy tiếp theo") thay vì tự quyết
định có nên switch hay không bằng một logic riêng.

Ranh giới ba lớp cần giữ rõ trong toàn bộ mục này:

- **Event** (tick, IPC, ISR) chỉ đổi `tcb->state` và vị trí trong list.
- **Scheduler** (`rtos_scheduler_select_next()`) chỉ quyết định *ai kế
  tiếp*, dựa trên state/list hiện tại.
- **PendSV** mới thực sự thực hiện switch (save/restore CPU context).

## 7. SysTick, Delay, and Wake-Up

SysTick là nguồn thời gian định kỳ của kernel — mỗi lần nó fire,
`SysTick_Handler` (`port/ARM/cortex-m3/rtos_port.c`) chỉ gọi thẳng vào
`rtos_tick_handler()` (kernel), không tự làm gì khác:

```text
SysTick
   |
   v
rtos_tick_handler()
   |
   +-> tang system tick
   |
   +-> kiem tra task dang delay da toi wake_tick chua
   |
   +-> dua task het delay ve READY
   |
   +-> request PendSV
```

### `rtos_delay()` làm gì với task

```text
RUNNING
   |
   | rtos_delay(ticks)
   v
BLOCKED (block_reason = DELAY)
   |
   | remove khoi ready list, luu wake_tick
   v
cho thoi gian troi qua
   |
   | SysTick toi wake_tick
   v
READY
   |
   | insert lai ready list
   v
scheduler co the chon task nay
```

`rtos_delay(ticks)` (`kernel/src/task.c`) tính `wake_tick` bằng tick hiện
tại cộng `ticks`, chuyển `state = BLOCKED`, `block_reason = RTOS_BLOCK_DELAY`,
rồi `rtos_list_remove()` khỏi ready list — vì **ready list chỉ chứa những
task scheduler có quyền chọn chạy** (mục 3); một task đang delay chưa được
phép chạy, nên nó phải rời khỏi đó, nếu không scheduler có thể chọn nhầm nó
trước khi hết thời gian chờ.

### Task đang delay nằm ở đâu

MyRTOS không có delayed list riêng. Một task đang delay:

- không nằm trong `ready_lists[]`,
- không nằm trong wait list IPC nào,
- vẫn tồn tại như một phần tử của `task_table[]` (mảng phẳng mọi task, xem
  mục 3), với `state = BLOCKED`, `block_reason = RTOS_BLOCK_DELAY`, và
  `wake_tick` lưu đúng thời điểm được phép chạy lại.

```text
task_table[]                          ready_lists[]
+-- Task A   READY                    +-- Task A
+-- Task B   BLOCKED / DELAY / wake_tick=100
+-- Task C   RUNNING                  +-- Task C
+-- Idle     READY                    +-- Idle
```

### Vì sao quét `task_table[]` thay vì một delayed list riêng

`rtos_tick_handler()` mỗi tick quét tuyến tính `task_table[]`, tìm đúng
task có `state == BLOCKED`, `block_reason == RTOS_BLOCK_DELAY`, và
`wake_tick` đã tới hạn. MyRTOS là một RTOS nhỏ với số task giới hạn
(`RTOS_MAX_TASKS`) — một mảng phẳng quét tuyến tính đơn giản và dễ hiểu
hơn việc duy trì thêm một delayed list được sắp theo `wake_tick`, phù hợp
với mục tiêu học tập của MyRTOS. Đây là lựa chọn đơn giản-hoá có cân nhắc,
không phải giới hạn kỹ thuật.

### Vì sao cần `block_reason`

`state == BLOCKED` chỉ nói task hiện không được chạy — nó không nói task
đang chờ gì. Một task có thể BLOCKED vì nhiều lý do khác nhau:
`RTOS_BLOCK_DELAY`, `RTOS_BLOCK_SEMAPHORE`, `RTOS_BLOCK_QUEUE_SEND`,
`RTOS_BLOCK_QUEUE_RECEIVE`. Tick handler chỉ có thẩm quyền wake đúng một
trong số đó — `RTOS_BLOCK_DELAY`; các lý do BLOCKED còn lại phải được đúng
IPC tương ứng wake (mục 8). `block_reason` là thông tin tối thiểu để tick
handler biết nó có được phép động vào task này hay không.

### Vì sao hết delay lại request PendSV

Khi một task quay lại READY, tập hợp task scheduler có thể chọn đã thay
đổi — task đó có thể có priority cao hơn task đang RUNNING, hoặc chỉ đơn
giản tham gia round-robin cùng priority. `rtos_tick_handler()` không tự
biết (và không cần biết) điều đó ảnh hưởng thế nào — nó chỉ request PendSV
**vô điều kiện** mỗi tick khi `rtos_scheduler_running`, để scheduler tự
đánh giá lại "ai nên chạy" (mục 4 tự nhiên xử lý cả round-robin time-slice
lẫn preemption bằng cùng một lần quét).

| Thành phần | Vai trò |
| --- | --- |
| SysTick | Cung cấp nhịp thời gian |
| Tick logic (`rtos_tick_handler()`) | Wake task hết delay |
| Scheduler | Quyết định ai chạy tiếp theo |
| PendSV | Thực hiện chuyển task thật |

Chi tiết exception/register của SysTick/PendSV không lặp lại ở đây — xem
[`context_switch.md`](context_switch.md).

## 8. Scheduler Interaction with IPC

Semaphore và queue là nguồn BLOCKED/READY thứ hai, độc lập với delay. Khi
một task chờ semaphore/queue, nó chuyển động giữa hai state quen thuộc:

```text
RUNNABLE -> BLOCKED
BLOCKED  -> READY
```

Khi task block vì IPC: `ready_item` rời ready list, `wait_item` vào wait
list của semaphore/queue tương ứng, task chuyển `BLOCKED`. Khi IPC event
wake task: `wait_item` rời wait list, `ready_item` quay lại
`ready_lists[priority]`, task chuyển `READY`.

Scheduler cần quan tâm tới việc này vì IPC có thể làm **tập hợp task READY
thay đổi** đúng lúc nó không ngờ tới — một task priority cao đang BLOCKED
chờ semaphore/queue có thể được wake trở lại READY bất kỳ lúc nào, và
scheduler cần có cơ hội đánh giá lại xem ai nên chạy tiếp theo:

```text
Task High BLOCKED
    |
    | semaphore/queue event
    v
Task High READY
    |
    | request PendSV neu can reschedule
    v
Scheduler chon lai task chay tiep theo
```

Ở góc nhìn scheduler, semaphore/queue hoạt động nội bộ ra sao không quan
trọng — nó chỉ nhìn thấy đúng hai thay đổi: task state và ready-list
membership, y hệt như với delay (mục 7). Chi tiết wait list, thuật toán
chọn waiter, direct handoff, ring buffer, hay API an toàn cho ISR đều thuộc
phạm vi của [`ipc.md`](ipc.md), không phải của scheduler.

Dù event tới từ delay hay từ IPC, context switch thật vẫn được thực hiện
bởi `PendSV_Handler`.

## 9. Scheduler vs Context Switch

| | Scheduler | Context Switch |
| --- | --- | --- |
| Câu hỏi trả lời | Task nào chạy tiếp theo? | Làm sao CPU thực sự chuyển sang task đó? |
| Vị trí | `kernel/src/core.c` | `port/ARM/cortex-m3/rtos_port.c` |
| Hàm chính | `rtos_scheduler_select_next()` | `PendSV_Handler` |
| Thao tác trên | `rtos_current_tcb`, `tcb->state`, `ready_lists[]` | PSP, R4-R11, exception frame |
| Biết gì về CPU | Không gì cả — thuần C, không CMSIS/STM32 | Toàn bộ cơ chế Cortex-M3 (SVC/PendSV/PSP/MSP) |

Hai lớp này tách biệt có chủ đích: kernel (bao gồm scheduler) giữ đúng
nguyên tắc CPU-độc lập — `rtos_scheduler_select_next()` không include bất
kỳ header CMSIS/STM32 nào, có thể đọc hiểu và test mà không cần biết gì về
Cortex-M3. Toàn bộ kiến thức "làm sao chuyển CPU sang task khác" nằm gọn
trong `port/ARM/cortex-m3/`, được gọi ra từ đúng một điểm nối:
`PendSV_Handler` gọi `rtos_scheduler_select_next()` ở giữa hai nửa
save/restore của chính nó (xem [`context_switch.md`](context_switch.md)
mục 11-12) — scheduler không hề biết PendSV, PSP hay EXC_RETURN tồn tại.
