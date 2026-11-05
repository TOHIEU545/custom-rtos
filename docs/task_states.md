# MyRTOS Task States

## 1. Overview

State của một task biểu diễn: **task này hiện có được scheduler chọn chạy
hay không, và nếu không thì vì lý do gì**.

```text
              scheduler chon
   READY ---------------------------> RUNNING
     ^                                   |
     |                                   |
     | wake (tick / IPC)                 | rtos_delay() / cho IPC
     |                                   v
     +----------------------------- BLOCKED
```

(Trước đó còn một state không nằm trong vòng lặp trên: `RTOS_TASK_UNUSED` —
slot TCB chưa từng được tạo, chuyển sang `READY` đúng một lần duy nhất qua
`rtos_task_create_static()`, không bao giờ quay lại `UNUSED`.)

Cần phân biệt hai khái niệm liên quan nhưng không đồng nhất:

- **State** (`tcb->state`) — trạng thái logic của task, một trong bốn giá
  trị của `rtos_task_state_t` (`kernel/include/rtos.h`).
- **Ready-list membership** (`tcb->ready_item.container`) — cách kernel tổ
  chức task theo priority để chọn task tiếp theo (chi tiết:
  [`scheduler.md`](scheduler.md)).

Hai thứ này đi cùng nhau nhưng không phải một: `READY` và `RUNNING` **cùng**
nằm trong ready list; chỉ `BLOCKED` mới thật sự rời khỏi đó (mục 3).

## 2. READY, RUNNING, and BLOCKED

| State | Ý nghĩa | Scheduler có thể chọn? | Nằm trong ready list? |
| --- | --- | --- | --- |
| `RTOS_TASK_READY` | Sẵn sàng chạy, đang chờ tới lượt | Có | Có |
| `RTOS_TASK_RUNNING` | Đang là task hiện hành (`rtos_current_tcb`) | (đang là kết quả chọn) | Có |
| `RTOS_TASK_BLOCKED` | Đang chờ tick hoặc IPC, chưa chạy được | Không | Không |

Nuance quan trọng nhất của MyRTOS: **`READY` và `RUNNING` đều nằm trong cùng
một `ready_lists[priority]`** — không có danh sách "currently running"
riêng. Task RUNNING chỉ khác task READY ở đúng một field: `tcb->state`.
Khi `rtos_scheduler_select_next()` chạy, nó demote task RUNNING hiện tại về
`READY` ngay tại chỗ (không đụng list), quét lại ready list, rồi đánh dấu
task được chọn là `RUNNING`.

Tại sao vẫn cần phân biệt `READY`/`RUNNING` nếu cả hai cùng nằm trong ready
list? Vì ready list chỉ trả lời "tập hợp task nào *có thể* chạy" — nó không
tự nói task nào *đang thực sự* sở hữu CPU tại thời điểm này. Kernel/port vẫn
cần một câu trả lời rõ ràng cho câu hỏi đó (ví dụ: `rtos_delay()` phải assert
`rtos_current_tcb->state == RTOS_TASK_RUNNING` trước khi cho một task tự
block chính nó). `tcb->state` là nơi giữ câu trả lời đó; ready list chỉ giữ
*tập hợp ứng viên*. Cơ chế PendSV dùng thông tin này thế nào thuộc phạm vi
[`context_switch.md`](context_switch.md), không nhắc lại ở đây.

## 3. State Transitions

| Transition | Ai gây ra | Tại sao |
| --- | --- | --- |
| `UNUSED → READY` | `rtos_task_create_static()` | Task tĩnh vừa được tạo, sẵn sàng chạy ngay |
| `READY → RUNNING` | `rtos_scheduler_select_next()` (scheduler) | Scheduler chọn task này làm task hiện hành |
| `RUNNING → READY` | `rtos_scheduler_select_next()` (scheduler) | Task hiện tại bị demote trước khi scheduler chọn lại (round-robin hoặc nhường cho task priority cao hơn) |
| `RUNNING → BLOCKED` | Chính task đó tự gọi: `rtos_delay()`, hoặc `rtos_sem_take()`/`rtos_queue_send()`/`rtos_queue_receive()` (qua `rtos_task_block_current()`) khi điều kiện chưa thoả | Task chủ động chờ thời gian hoặc chờ IPC |
| `BLOCKED → READY` | Tick handler (`rtos_tick_handler()`) nếu `block_reason == DELAY`; hoặc IPC give/send/receive (qua `rtos_task_make_ready()`) nếu `block_reason` là semaphore/queue | Điều kiện task đang chờ đã thoả |

Chỉ đúng 5 transition này tồn tại trong source hiện tại — không có
`READY → BLOCKED` trực tiếp (một task không tự block một task khác; nó chỉ
tự block chính nó khi đang `RUNNING`), và không có transition nào quay lại
`UNUSED`.

## 4. Why BLOCKED Tasks Leave the Ready List

Ready list chỉ chứa những task mà scheduler **có quyền chọn chạy** ngay bây
giờ (mục 2, [`scheduler.md`](scheduler.md) mục 3). Một task `BLOCKED` chưa
đủ điều kiện chạy — nó đang chờ một thứ gì đó (hết thời gian delay, hoặc
một event IPC) — nên `ready_item` của nó phải bị `rtos_list_remove()` khỏi
`ready_lists[priority]` ngay khi chuyển sang `BLOCKED`:

```text
RUNNING
   |
   | block (delay hoac cho IPC)
   v
BLOCKED
   |
   +-> ready_item bi remove khoi ready list
```

Nếu bỏ qua bước này, task `BLOCKED` vẫn còn nằm trong ready list, và
`rtos_scheduler_select_next()` — vốn chỉ quét ready list mà không kiểm tra
lại `state` của từng phần tử — có thể chọn nhầm một task chưa đủ điều kiện
chạy.

## 5. Block Reasons

`state == RTOS_TASK_BLOCKED` chỉ nói được một điều: *task này hiện không
được chạy*. Nó không nói task đang chờ cái gì — mà đây lại là thông tin bắt
buộc phải biết, vì các cơ chế wake khác nhau chỉ có thẩm quyền wake đúng
loại BLOCKED của mình. `block_reason` (`rtos_block_reason_t`,
`kernel/include/rtos.h`) chính là thông tin đó:

| `block_reason` | Ai được phép wake |
| --- | --- |
| `RTOS_BLOCK_DELAY` | Tick handler (`rtos_tick_handler()`), khi `wake_tick` tới hạn |
| `RTOS_BLOCK_SEMAPHORE` | `rtos_sem_give()`/`rtos_sem_give_from_isr()` |
| `RTOS_BLOCK_QUEUE_SEND` | `rtos_queue_receive()` (có chỗ trống để gửi tiếp) |
| `RTOS_BLOCK_QUEUE_RECEIVE` | `rtos_queue_send()`/`rtos_queue_send_from_isr()` (có item để nhận) |

(`RTOS_BLOCK_NONE` là giá trị mặc định khi task không `BLOCKED` — không
phải một lý do block thật.) Thuật toán chọn waiter và cơ chế give/send bên
trong semaphore/queue không nhắc lại ở đây — xem [`ipc.md`](ipc.md).

## 6. Delay Blocking vs IPC Blocking

| | Delay | IPC (semaphore/queue) |
| --- | --- | --- |
| Điều kiện chờ | Tick hệ thống chạm `wake_tick` | Semaphore/queue có event (give/send/receive) |
| `ready_item` | Rời ready list | Rời ready list |
| `wait_item` | Không dùng (`container == NULL`) | Nằm trong wait list của đúng object IPC |
| Nơi wake | Tick handler quét `task_table[]` | Chính object semaphore/queue đó |

**Delay**: `rtos_delay()` set `state = BLOCKED`, `block_reason =
RTOS_BLOCK_DELAY`, lưu `wake_tick`, gỡ `ready_item` khỏi ready list. TCB vẫn
còn trong `task_table[]` — MyRTOS không có delayed list riêng (xem
[`scheduler.md`](scheduler.md) mục 7).

**IPC**: `rtos_task_block_current(wait_list, reason)` set `state =
BLOCKED`, gỡ `ready_item` khỏi ready list, và chèn `wait_item` vào đúng
`wait_list` do semaphore/queue sở hữu. Khi object đó có event phù hợp, nó tự
tìm waiter trong `wait_list` của mình và gọi `rtos_task_make_ready()` —
task không tự "thức dậy", nó được đúng object nó đang chờ đánh thức. Chi
tiết thuật toán chọn waiter, direct handoff, ring buffer thuộc về
[`ipc.md`](ipc.md).
