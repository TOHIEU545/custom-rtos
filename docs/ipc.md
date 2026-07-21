# NTOS IPC

## 1. Overview

IPC ở đây không chỉ là "truyền dữ liệu giữa các task" — nó là cơ chế đồng
bộ hoá: cho phép một task chờ một điều kiện do task khác (hoặc ISR) tạo ra,
mà không cần busy-wait. NTOS hiện có hai cơ chế:

```text
Semaphore -> bao hieu / dong bo (mot token co hay khong)
Queue     -> truyen du lieu + dong bo (item co hay khong, cho co hay khong)
```

Cả hai đều có thể làm một task chuyển giữa hai nhóm state đã biết ở
[`task_states.md`](task_states.md):

```text
RUNNABLE -> BLOCKED   (take/receive khong co san, hoac send vao queue day)
BLOCKED  -> READY     (give/send/receive tao ra dieu kien task dang cho)
```

Nhưng **IPC không tự thực hiện scheduling hay context switch** — nó chỉ
đổi `tcb->state` và list membership. Việc chọn task chạy tiếp theo vẫn là
[`scheduler.md`](scheduler.md); việc thực sự chuyển CPU vẫn là
[`context_switch.md`](context_switch.md) (`PendSV_Handler`). Tài liệu này
chỉ nói về phần ở giữa: semaphore/queue nội bộ tổ chức thế nào để biết ai
đang chờ mình, và tại sao phải tổ chức như vậy.

## 2. Why IPC Objects Need Their Own Wait Lists

Một hệ thống có nhiều semaphore/queue độc lập cùng lúc — ví dụ:

```text
Semaphore S1: Task A wait, Task B wait
Semaphore S2: Task C wait
```

Khi `ntos_sem_give(&S1)` chạy, nó chỉ cần biết một điều: **ai đang chờ đúng
S1** — không phải "có task nào đang BLOCKED trong toàn hệ thống không".
Đây là lý do mỗi object IPC tự giữ một wait list riêng
(`ntos_semaphore_t::waiting_tasks`, `ntos_queue_t::waiting_receivers`/
`waiting_senders`):

```text
S1.waiting_tasks -> Task A -> Task B
S2.waiting_tasks -> Task C
```

Nếu không có wait list riêng theo từng object, thứ duy nhất kernel biết về
một task là `state == BLOCKED` — thông tin đó không phân biệt được task
đang chờ `S1`, chờ `S2`, hay chờ một queue nào khác. Wait list chính là câu
trả lời cho câu hỏi **"task nào đang chờ đúng object IPC nào"** — một quan
hệ mà `block_reason` (chỉ phân biệt *loại* IPC, xem
[`task_states.md`](task_states.md) mục 5) không đủ để biểu diễn.

## 3. Why TCB Has a Separate `wait_item`

TCB có hai node list độc lập:

```c
ntos_list_item_t ready_item;  /* node trong ready_lists[priority] */
ntos_list_item_t wait_item;   /* node trong wait list cua sem/queue */
```

```text
READY/RUNNING              BLOCKED vi IPC
  ready_item -> ready list    ready_item -> khong gan list nao
  wait_item  -> khong gan     wait_item  -> wait list cua sem/queue
```

Một `ntos_list_item_t` (intrusive, xem [`scheduler.md`](scheduler.md) mục
3) chỉ có đúng một cặp `next`/`previous`, nên chỉ có thể thuộc **một** list
tại một thời điểm. Ready list và wait list là hai vai trò quản lý hoàn toàn
khác nhau — "task này có được scheduler chọn không" và "task này đang chờ
object IPC nào" — nếu dùng chung một node cho cả hai vai trò, node đó không
thể đồng thời nằm trong `ready_lists[priority]` lẫn `waiting_tasks` của một
semaphore, tức không thể biểu diễn được trạng thái trung gian nào một cách
rõ ràng. Tách thành `ready_item` và `wait_item` giải quyết đúng vấn đề đó:
mỗi node là "vé tham gia" của đúng một list, độc lập với node còn lại.

Tại một thời điểm, đúng một trong hai node đang gắn vào một list — không
bao giờ cả hai cùng lúc; đây là bất biến `NTOS_ASSERT` ở cả
`ntos_task_block_current()` và `ntos_task_make_ready()` (mục 6).

## 4. Semaphore

`ntos_semaphore_t` (`include/ntos/ntos_sem.h`) chỉ có ba field: `count`
(số token hiện có), `max_count` (giới hạn trên), và `waiting_tasks`.

```text
ntos_sem_take(sem, timeout)
   |
   +-> count > 0        -> count--, return OK
   |
   +-> count == 0
          |
          +-> timeout == NO_WAIT -> return TIMEOUT
          |
          +-> block: wait_item vao sem->waiting_tasks, state = BLOCKED
                 |
                 v
          (duoc give() wake truc tiep) -> return OK
```

```text
ntos_sem_give(sem)
   |
   +-> co waiter trong waiting_tasks
   |      -> chon waiter priority cao nhat, make_ready() truc tiep
   |      -> (khong dong vao count)
   |
   +-> khong co waiter
          -> count < max_count ? count++ : ERR_RESOURCE_FULL
```

Điểm đáng chú ý nhất: `ntos_sem_give()` dùng **direct handoff** — nếu có
waiter, token được giao thẳng cho waiter đó (chỉ gọi
`ntos_task_make_ready()`), `count` không hề bị đụng vào. Lý do: nếu vừa
tăng `count` vừa đồng thời wake waiter, cùng một token sẽ bị tính hai lần —
một lần nằm trong `count`, một lần đã trao cho waiter — mở đường cho một
`take()` khác cướp nhầm phần chưa thuộc về nó. Direct handoff đảm bảo một
`give()` tương ứng chính xác với một lần "có token thêm", dù token đó đi
vào `count` hay đi thẳng vào tay một waiter.

## 5. Queue

`ntos_queue_t` (`include/ntos/ntos_queue.h`) là một ring buffer tĩnh
(`storage`, `item_size`, `capacity`, `read_index`, `write_index`, `count`),
cộng **hai** wait list riêng: `waiting_receivers` và `waiting_senders`.

Hai wait list tồn tại vì queue có hai điều kiện chờ hoàn toàn khác nhau:
`waiting_receivers` trả lời "ai đang chờ có item để đọc" (queue rỗng),
`waiting_senders` trả lời "ai đang chờ có chỗ trống để ghi" (queue đầy) —
gộp chung một list sẽ không phân biệt được nên wake ai khi chỉ một trong
hai điều kiện đó thay đổi.

```text
ntos_queue_receive()                    ntos_queue_send()
   |                                       |
   +-> count > 0 -> doc item, count--      +-> count < capacity -> ghi item, count++
   |      (co the wake 1 sender)           |      (co the wake 1 receiver)
   |                                       |
   +-> count == 0                         +-> count == capacity
          +-> NO_WAIT -> EMPTY                   +-> NO_WAIT -> FULL
          +-> block vao waiting_receivers        +-> block vao waiting_senders
```

Khác với semaphore, `send()`/`receive()` **không** direct-handoff item/chỗ
trống — chúng chạy trong một vòng lặp, và sau khi được wake, task tự kiểm
tra lại điều kiện (`count > 0`/`count < capacity`) trước khi thao tác buffer
thật. Lý do: được wake chỉ nghĩa là "một chỗ/một item *vừa* rảnh ra", không
phải "đã được giữ chỗ cho task này" — một task khác (được lập lịch trước)
có thể đã giành mất phần đó. Đây là **wake-and-retry**, khác hẳn direct
handoff của semaphore — vì bản thân dữ liệu (item, hoặc chỗ trống trong
ring buffer) không thể "giao tay" giống một token đếm được.

## 6. Blocking and Wake-Up

Semaphore và queue dùng chung đúng hai helper (`kernel/ntos_task.c`):

```text
Block (RUNNING -> BLOCKED)          Wake (BLOCKED -> READY)
  ready_item removed                  wait_item removed
  wait_item -> object's wait list     ready_item -> ready_lists[priority]
  state = BLOCKED                     state = READY
```

`ntos_task_block_current()`/`ntos_task_make_ready()` chỉ đổi state và list
membership — chúng **không** tự tạo critical section và **không** tự
request PendSV; caller (semaphore/queue) tự bọc critical section quanh
chúng và tự request PendSV sau khi rời critical section, cùng quy ước với
`ntos_delay()` ([`scheduler.md`](scheduler.md) mục 7).

Sau khi một task được wake, việc request PendSV có cần thiết hay không phụ
thuộc priority: nếu waiter vừa được wake có priority cao hơn task đang
chạy, tập hợp task READY vừa thay đổi theo cách scheduler cần biết ngay —
nên PendSV được request để scheduler đánh giá lại. Thuật toán chọn waiter
theo priority và chi tiết `ntos_scheduler_select_next()` không nhắc lại ở
đây — xem [`scheduler.md`](scheduler.md).

## 7. IPC from ISR

`ntos_sem_give_from_isr()`/`ntos_queue_send_from_isr()` tồn tại vì ISR có
một ràng buộc tuyệt đối: **không được block**. Timeout không có ý nghĩa
trong ngữ cảnh ISR — nó chỉ thực hiện đúng phần "give/send không chờ",
không bao giờ gọi `ntos_task_block_current()`.

```text
ISR
 |
 +-> give/send
 |
 +-> co waiter? -> make_ready(waiter)
 |
 +-> waiter priority cao hon task hien tai? -> request PendSV
 |
 v
return tu ISR
 |
 v
PendSV chay (neu duoc request) -> scheduler chon lai
```

Một khác biệt với đường task-context: biến thể `_from_isr` phải kiểm tra
`ntos_scheduler_running` trước khi so priority với `ntos_current_tcb` — vì
ISR có thể fire trước khi task đầu tiên từng được chọn, lúc đó chưa có
"task hiện tại" hợp lệ nào để so sánh. Context switch thật **không** xảy ra
ngay trong ISR — nó chỉ được request; việc chuyển CPU thật sự vẫn đợi tới
`PendSV_Handler` sau khi ISR return (xem
[`context_switch.md`](context_switch.md)).
