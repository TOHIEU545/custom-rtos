# Roadmap phát triển lại MyRTOS từ phiên bản nhỏ nhất

## 1. Tư tưởng của lần viết lại

Roadmap này không đi theo thứ tự các file của MyRTOS hiện tại. Source hiện tại
là **kết quả cuối của nhiều quyết định**, còn mục tiêu lần viết lại là tự đi qua
những vấn đề đã dẫn tới các quyết định đó.

Vì vậy mỗi phiên bản phải tuân theo vòng lặp:

```text
Viết cách đơn giản nhất
        |
        v
Chạy được và kiểm thử
        |
        v
Quan sát hạn chế thật sự
        |
        v
Thêm đúng một cơ chế để giải quyết hạn chế đó
```

Một số code ở giai đoạn đầu sẽ bị thay thế ở giai đoạn sau. Đây là chủ ý, không
phải làm thừa. Nếu viết ngay ready list, wait list và generic blocking helper,
ta chỉ biết chúng hoạt động chứ chưa hiểu tại sao chúng đáng tồn tại.

Đặc biệt, **linked list không được viết ở đầu roadmap**. Ban đầu scheduler và
IPC sẽ quét `task_table[]`. Chỉ sau khi chính cách quét này gây lặp code và làm
lộ nhu cầu insert/remove/round-robin, ta mới thiết kế intrusive linked list.

## 2. Đích cuối cùng vẫn phải đủ tính năng

Kết thúc roadmap, bản viết lại phải có toàn bộ phạm vi của project hiện tại:

- Static allocation, không heap.
- Task entry, TCB, stack riêng và tạo task tĩnh.
- Initial Cortex-M3 context frame; task dùng PSP, handler dùng MSP.
- Khởi động task đầu tiên bằng SVC.
- Context switch bằng PendSV; tick bằng SysTick.
- Cooperative yield và fixed-priority preemptive scheduling.
- Round-robin giữa task cùng priority và idle task.
- Trạng thái `UNUSED`, `READY`, `RUNNING`, `BLOCKED` cùng block reason.
- Delay chống tick wraparound.
- Intrusive doubly-linked circular list có sentinel và cursor.
- Ready list theo priority và wait list cho IPC.
- Stack fill, high-water mark và guard.
- Counting semaphore trong task context và ISR context.
- Queue FIFO tĩnh; block sender/receiver; send từ ISR.
- Critical section bằng PRIMASK; assert/panic và kiểm tra invariant.
- Toàn bộ examples, module tests, integration test và stress test hiện có.

Không thêm tính năng ngoài phạm vi trong lần ôn lại: chưa có mutex, task
delete/suspend/resume, dynamic allocation, timed IPC hữu hạn, tickless idle,
ESP32 hay SMP.

## 3. Cách tổ chức quá trình

- Giữ bản hiện tại làm baseline và tạo nhánh Git riêng để viết lại.
- Xem BSP, startup, linker script và firmware bare-metal STM32F103 hiện tại là
  nền móng đã hoạt động; lần ôn này bắt đầu trực tiếp từ kernel/port.
- Sau mỗi giai đoạn tạo một commit/tag có thể build và chạy độc lập.
- Chỉ xem public header, tài liệu và test trước. Sau khi tự làm xong mới đối
  chiếu implementation cũ.
- Các giai đoạn đầu có thể dùng test/demo tạm thời rất nhỏ. Bộ test hiện tại
  kiểm tra kiến trúc cuối nên không bắt buộc tất cả phải build ngay từ ngày đầu.
- Khi refactor, test cũ phải tiếp tục pass. Refactor không được âm thầm đổi hành
  vi đã có.
- Với mỗi giai đoạn, tự ghi lại ba câu: vấn đề vừa gặp là gì, giải pháp vừa thêm
  giải quyết nó thế nào, và invariant mới là gì.

## 4. Lộ trình tiến hóa

### Giai đoạn 0 — Giữ một baseline đáng tin cậy

**Mục đích:** Biết chắc bản cũ chạy thế nào trước khi tự viết lại.

**Thực hiện:**

- Clean build toàn bộ target hiện tại.
- Chạy ít nhất `test_first_task` và `test_integration_full_demo` trên board.
- Lưu output UART, kích thước ELF và commit/hash hiện tại.
- Tạo nhánh viết lại; không xóa bản đang hoạt động.

**Yêu cầu hoàn thành:** Có thể quay lại bản cũ để phân biệt lỗi mới với lỗi vốn
có, nhưng không dùng source cũ như đoạn code để chép.

---

### Giai đoạn 1 — Một task có stack riêng và initial context

**Câu hỏi cần hiểu:** Làm sao CPU bắt đầu một hàm chưa từng được gọi bằng lệnh
`bl`, trên một stack hoàn toàn mới?

**Thực hiện:**

- Định nghĩa kiểu task entry `typedef void (*rtos_task_entry_t)(void *argument)`;
  ở đây nó chỉ là kiểu địa chỉ được ghi vào PC của initial frame.
- Tạo TCB tối thiểu chỉ có `stack_pointer`.
- Caller cấp phát tĩnh stack cho task, căn chỉnh 8 byte.
- Viết `rtos_port_stack_init()` dựng frame 16 word:
  - R4-R11 là software frame.
  - R0-R3, R12, LR, PC, xPSR là hardware frame giả.
- Đặt `R0 = argument`, `PC = entry`, Thumb bit trong xPSR.
- Đặt LR về một task-return trap.

**Chưa làm:** Scheduler và PendSV.

**Kiểm thử:** Dùng test kiểm tra trực tiếp từng slot của frame; sau này đối
chiếu với `test_initial_stack`.

**Yêu cầu hoàn thành:** Tự vẽ được frame và giải thích vì sao PC làm task bắt
đầu chạy, tại sao argument phải nằm ở R0 và tại sao cần 8-byte alignment.

**Hạn chế dẫn tới bước sau:** Context đã nằm trên RAM nhưng CPU chưa restore nó.

---

### Giai đoạn 2 — Chạy task đầu tiên bằng SVC

**Câu hỏi cần hiểu:** Ai lấy context giả từ stack và biến nó thành register thật?

**Thực hiện:**

- Có `rtos_current_tcb` trỏ tới task đầu tiên.
- `rtos_start()` gọi port start; port phát `svc 0`.
- `SVC_Handler` restore R4-R11, nạp PSP và exception-return với `0xFFFFFFFD`.
- Task chạy Thread mode bằng PSP; SVC chạy Handler mode bằng MSP.

**Yêu cầu hành vi:**

- Không gọi task entry trực tiếp trong C.
- Task nhận đúng argument và chạy bằng PSP.
- `rtos_start()` không return.
- Không đánh dấu scheduler đang chạy trước khi PSP hợp lệ.

**Kiểm thử cuối:** `test_first_task`.

**Phải giải thích được:** SVC không phải scheduler; nó chỉ là cơ chế bootstrap
context đầu tiên.

**Hạn chế dẫn tới bước sau:** Một task đã chạy được nhưng chưa có cách dừng nó
tạm thời để chạy task khác.

---

### Giai đoạn 3 — Hai task, context switch hard-code bằng PendSV

**Câu hỏi cần hiểu:** Context switch thật sự cần lưu gì và tại sao dùng PendSV?

**Thực hiện đơn giản có chủ ý:**

- Tạo đúng hai TCB toàn cục: task A và task B.
- `rtos_yield()` chỉ pend PendSV.
- `PendSV_Handler`:
  1. Lấy PSP của task cũ.
  2. Lưu R4-R11; hardware đã lưu phần còn lại.
  3. Lưu PSP vào TCB cũ.
  4. Nếu current là A thì chọn B, ngược lại chọn A.
  5. Restore TCB mới và exception-return.
- Chưa có generic scheduler, priority hay list.

**Yêu cầu hoàn thành:** Hai task yield qua lại lâu dài; biến local/register và
hai stack không lẫn nhau. Tự giải thích được hardware frame, software frame và
vai trò `tcb->stack_pointer`.

**Hạn chế dẫn tới bước sau:** Logic `A <-> B` không mở rộng được khi số task
thay đổi.

---

### Giai đoạn 4 — N task bằng bảng task và round-robin tuyến tính

**Câu hỏi cần hiểu:** Làm sao bỏ hard-code hai task bằng cấu trúc đơn giản nhất?

**Thực hiện:**

- Thêm `task_table[RTOS_MAX_TASKS]`, `task_count`, `current_index`.
- Mở rộng TCB với entry, argument, name và stack metadata.
- Viết `rtos_task_create_static()` để dựng frame rồi đăng ký task vào bảng.
- Scheduler chỉ tăng index và quay về 0 khi tới cuối bảng.
- Vẫn chỉ cooperative qua `yield()`.

**Chưa làm:** Priority, BLOCKED, ready list và linked list.

**Yêu cầu hoàn thành:** 3 task trở lên round-robin đúng; từ chối task quá giới
hạn, entry/stack chưa hợp lệ và tạo lại TCB đã dùng.

**Kiểm thử:** Biến thể đơn giản của `test_scheduler_cooperative`.

**Hạn chế dẫn tới bước sau:** Task không gọi yield có thể chiếm CPU mãi mãi.

---

### Giai đoạn 5 — SysTick tạo preemption

**Câu hỏi cần hiểu:** Làm sao lấy lại CPU từ một task không tự nguyện yield?

**Thực hiện:**

- Cấu hình SysTick từ `cpu_clock_hz / RTOS_TICK_RATE_HZ`.
- SysTick tăng `rtos_tick_count` và pend PendSV.
- Đặt PendSV ở priority thấp nhất để context switch chỉ diễn ra sau ISR khác.
- Thêm `rtos_scheduler_running` để không switch trước khi task đầu tiên sẵn sàng.

**Yêu cầu hoàn thành:** Các task vòng lặp không yield vẫn cùng tiến triển;
SysTick không tự save/restore task và context switch thật vẫn chỉ ở PendSV.

**Kiểm thử:** Phiên bản round-robin của `test_scheduler_preemptive`.

**Hạn chế dẫn tới bước sau:** Mọi task luôn runnable; chưa thể ngủ/chờ nên vẫn
tốn CPU và chưa có khái niệm trạng thái.

---

### Giai đoạn 6 — Task state, delay và idle task bằng cách quét bảng

**Câu hỏi cần hiểu:** Scheduler phải làm gì với một task hiện chưa được phép
chạy?

**Thực hiện cách đơn giản trước:**

- Thêm state `UNUSED`, `READY`, `RUNNING`, `BLOCKED`.
- Thêm `wake_tick` và `RTOS_BLOCK_DELAY`.
- `rtos_delay()` đặt task hiện tại BLOCKED rồi pend PendSV.
- Mỗi tick quét toàn bộ `task_table[]` để wake task tới hạn.
- Scheduler quét bảng, bỏ qua task BLOCKED.
- Tạo idle task để luôn có một task runnable khi mọi application task delay.
- Dùng phép so sánh tick wraparound-safe; giới hạn delay `INT32_MAX` tick.

**Chưa làm:** Ready list; đây vẫn là scheduler O(n).

**Yêu cầu hoàn thành:**

- `delay(0)` tương đương yield.
- State transition đúng: `READY <-> RUNNING -> BLOCKED -> READY`.
- Không chọn task BLOCKED.
- Delay vẫn đúng khi tick đi qua `UINT32_MAX -> 0`.
- Idle không bao giờ block.

**Kiểm thử:** `test_task_delay` và các case wraparound nhỏ tự viết.

**Hạn chế dẫn tới bước sau:** Round-robin không thể biểu diễn mức độ quan trọng
khác nhau giữa các task.

---

### Giai đoạn 7 — Fixed priority nhưng vẫn quét `task_table[]`

**Câu hỏi cần hiểu:** Priority policy khác context switch như thế nào?

**Thực hiện:**

- Thêm priority cố định vào TCB.
- Scheduler quét toàn bộ bảng để tìm priority cao nhất đang READY/RUNNING.
- Dùng một cursor/index đơn giản để chia lượt giữa task cùng priority.
- Task priority cao vừa wake phải preempt task priority thấp.

**Chưa làm:** `ready_lists[priority]`.

**Yêu cầu hoàn thành:**

- Số priority lớn hơn nghĩa là ưu tiên cao hơn.
- Highest-priority runnable task luôn được chọn.
- Các task cùng priority round-robin.
- Idle có priority 0 và chỉ chạy khi cần.
- Nhận thức được starvation của fixed-priority là hành vi policy hiện tại.

**Kiểm thử:** `test_scheduler_priority` và `test_scheduler_preemptive`.

**Hạn chế cần ghi lại:** Mỗi context switch phải quét mọi task; logic fairness
trong mảng bắt đầu khó đọc. Chưa sửa ngay — cần gặp thêm vấn đề IPC trước.

---

### Giai đoạn 8 — Stack diagnostics

**Câu hỏi cần hiểu:** Làm sao biết stack tĩnh được cấp đủ hay đã tràn?

**Thực hiện:**

- Fill stack bằng `0xA5A5A5A5` trước khi dựng initial frame.
- Lưu `stack_low`, `stack_high`, `stack_words`.
- Đo untouched words và high-water mark từ vùng fill liên tục ở đáy.
- Dùng word thấp nhất làm guard.
- Tôn trọng compile-time switch cho fill và guard.

**Yêu cầu hoàn thành:** `test_stack_diagnostics` pass; stack pointer ngoài vùng
hợp lệ và guard bị ghi đè được phát hiện.

**Hạn chế dẫn tới bước sau:** Task mới chỉ chờ thời gian, chưa thể chờ event từ
task khác.

---

### Giai đoạn 9 — Semaphore phiên bản đầu, chưa dùng wait list

**Câu hỏi cần hiểu:** Task chờ một tài nguyên khác gì task delay?

**Thực hiện cách đơn giản có chủ ý:**

- Counting semaphore chỉ có `count` và `max_count`.
- Thêm block reason `SEMAPHORE` và một `waiting_object` tạm thời trong TCB.
- Khi take không có token, task ghi `waiting_object = sem`, chuyển BLOCKED.
- Khi give, quét toàn bộ `task_table[]` để tìm waiter của semaphore có priority
  cao nhất rồi wake nó.
- Dùng direct handoff: có waiter thì không tăng count.
- `NO_WAIT` trả TIMEOUT; mọi timeout khác 0 vẫn là chờ vô hạn như project cũ.
- Bảo vệ count/state bằng save/restore PRIMASK.

**Yêu cầu hoàn thành:**

- Không mất hoặc nhân đôi token.
- Không wake task đang chờ object khác.
- Waiter priority cao nhất được chọn.
- `max_count` và `ERR_RESOURCE_FULL` đúng.

**Kiểm thử:** `test_semaphore_basic`, sau đó `test_semaphore_blocking` khi API
đã tương thích.

**Hạn chế cần quan sát:** `give()` phải biết và quét toàn bộ task của kernel;
TCB phải chứa con trỏ object chung chung. Với nhiều IPC object, cách này bắt đầu
liên kết chặt và lặp logic tìm waiter.

---

### Giai đoạn 10 — Queue phiên bản đầu, tiếp tục quét bảng

**Câu hỏi cần hiểu:** Queue cần chờ những điều kiện nào và khác semaphore ở đâu?

**Thực hiện:**

- Ring buffer tĩnh: storage, item size, capacity, read/write index, count.
- FIFO send/receive và wrap index.
- Thêm block reason `QUEUE_SEND` và `QUEUE_RECEIVE`.
- Task BLOCKED vẫn ghi `waiting_object = queue`.
- Khi send thành công, quét task table để wake receiver priority cao nhất.
- Khi receive thành công, quét để wake sender priority cao nhất.
- Dùng wake-and-retry; không reserve item/slot chỉ vì task đã được wake.
- `NO_WAIT` trả FULL/EMPTY; timeout khác 0 là chờ vô hạn.

**Yêu cầu hoàn thành:** FIFO, wraparound, block hai chiều, nhiều waiter và việc
re-check sau wake đều đúng.

**Kiểm thử:**

- `test_queue_fifo`
- `test_queue_blocking_receive`
- `test_queue_blocking_send`

**Nút thắt phải nhìn thấy trước khi đi tiếp:**

- Scheduler quét toàn bộ task để tìm runnable task.
- Tick quét toàn bộ task để tìm delayed task.
- Semaphore/queue lại quét toàn bộ task để tìm waiter của từng object.
- Logic block/wake bị lặp ở delay, semaphore và queue.
- Việc xóa/chèn một task vào tập runnable/waiter chưa có biểu diễn trực tiếp.

Đến đây nhu cầu về linked list mới xuất hiện từ code thật, không phải từ việc
nhìn kiến trúc có sẵn.

---

### Giai đoạn 11 — Thiết kế linked list để giải quyết nút thắt đã gặp

**Câu hỏi cần trả lời trước khi code:**

- Vì sao cần remove một task đã biết địa chỉ trong O(1)?
- Vì sao round-robin phù hợp với circular list?
- Vì sao sentinel giúp bỏ các nhánh đặc biệt đầu/cuối/rỗng?
- Vì sao không dùng heap và node phải nằm trong TCB?
- Vì sao một task cuối cùng cần hai node `ready_item` và `wait_item`?

**Sau khi trả lời mới thực hiện:**

- Intrusive doubly-linked circular list.
- Sentinel `end`, cursor `index`, `length`.
- List item có `next`, `previous`, `owner`, `container`.
- `init`, `item_init`, `push_back`, `remove`, `next_owner`, `is_empty`.

**Kiểm thử độc lập:**

- List 0, 1 và nhiều item.
- Insert/remove đầu, giữa, cuối và item duy nhất.
- Double insert bị chặn.
- Remove item mà cursor đang trỏ tới vẫn giữ vòng lặp hợp lệ.
- `next_owner()` quay vòng công bằng và bỏ qua sentinel.

**Yêu cầu hoàn thành:** Không tích hợp ngay trong cùng commit. Trước tiên list
phải được hiểu và test như một cấu trúc dữ liệu độc lập.

---

### Giai đoạn 12 — Thay scheduler quét bảng bằng ready list

**Câu hỏi cần hiểu:** Linked list vừa viết cải thiện đúng phần nào của scheduler?

**Thực hiện refactor, không đổi policy:**

- Tạo `ready_lists[RTOS_MAX_PRIORITIES]`.
- Thêm `ready_item` vào TCB; owner trỏ về TCB.
- Task create chèn task vào ready list tương ứng.
- Scheduler quét số priority cố định từ cao xuống thấp, không quét mọi task.
- `next_owner()` cung cấp round-robin cursor riêng cho từng priority.
- RUNNING vẫn nằm trong ready list; BLOCKED bị remove.
- Giữ `task_table[]` chỉ để quản lý số task và quét delayed task mỗi tick.

**Yêu cầu hoàn thành:** Các test cooperative, preemptive, priority và delay đã
pass trước refactor vẫn pass; thêm:

- `test_scheduler_ready_list_priority`
- `test_scheduler_ready_list_blocking`

**Phải giải thích được:** Vì sao vẫn còn `task_table[]`; linked list không được
dùng chỉ vì “list tốt hơn array”, mà mỗi cấu trúc có một trách nhiệm khác nhau.

---

### Giai đoạn 13 — Thay `waiting_object` và quét bảng bằng IPC wait list

**Câu hỏi cần hiểu:** Vì sao `ready_item` không thể tái sử dụng làm wait item?

**Thực hiện refactor:**

- Thêm `wait_item` riêng vào TCB.
- Semaphore có `waiting_tasks`.
- Queue có `waiting_senders` và `waiting_receivers`.
- Viết helper nội bộ:
  - `rtos_task_block_current(wait_list, reason)`.
  - `rtos_task_make_ready(task)`.
  - `rtos_waiter_select_highest(wait_list)`.
- Xóa cơ chế `waiting_object` tạm thời.
- Semaphore giữ direct handoff; queue giữ wake-and-retry.

**Invariant cuối:**

```text
READY/RUNNING:
    ready_item thuộc ready_lists[priority]
    wait_item.container == NULL

BLOCKED vì IPC:
    ready_item.container == NULL
    wait_item thuộc đúng wait list của object

BLOCKED vì delay:
    cả ready_item và wait_item đều không nằm trong list
```

**Yêu cầu hoàn thành:** Toàn bộ semaphore/queue test task-context vẫn pass;
không còn đường IPC nào quét `task_table[]` để tìm waiter.

**Phải giải thích được:** Linked list đã xuất hiện để giải quyết membership,
remove và round-robin cụ thể; không phải vì mọi RTOS bắt buộc phải dùng list.

---

### Giai đoạn 14 — API dùng từ ISR và deferred preemption

**Câu hỏi cần hiểu:** Tại sao ISR không được dùng API blocking và không được
context switch trực tiếp?

**Thực hiện:**

- `rtos_sem_give_from_isr()`.
- `rtos_queue_send_from_isr()`.
- Output `higher_priority_task_woken`.
- Có thể wake task trước scheduler start mà không dereference current TCB sai.
- Nếu wake task priority cao hơn, chỉ pend PendSV; switch sau khi ISR return.

**Yêu cầu hành vi:**

- ISR API tuyệt đối không block.
- Critical section khôi phục đúng PRIMASK trước đó.
- Queue full và semaphore full trả đúng status.
- Không tự thêm queue receive-from-ISR vì project hiện tại chưa có API đó.

**Kiểm thử:** `test_semaphore_isr`, `test_queue_isr`.

---

### Giai đoạn 15 — Hardening bằng invariant và lỗi bất thường

**Mục đích:** Chuyển từ “chạy được trong happy path” thành kernel dễ phát hiện
lỗi thiết kế và lỗi bộ nhớ.

**Thực hiện:**

- Hoàn thiện `RTOS_ASSERT` và `rtos_port_panic()`.
- Assert stack pointer nằm trong vùng stack trước restore.
- Assert state, block reason và list container trước/sau mọi transition.
- Kiểm tra invalid argument/state/priority/context và giới hạn task.
- Xác nhận task return đi vào trap.
- Kiểm tra critical section không để interrupt bị bật sai trạng thái cũ.

**Yêu cầu hoàn thành:** Mỗi invariant quan trọng có ít nhất một test hoặc một
cách cố ý phá để chứng minh assert bắt được lỗi.

---

### Giai đoạn 16 — Integration, stress và đóng gói lại project

**Mục đích:** Chứng minh mọi cơ chế hoạt động cùng nhau và bản mới thay thế
được bản hiện tại.

**Thực hiện:**

- Chạy lại toàn bộ test theo thứ tự ở mục 5.
- Chạy `test_full_system_stress` đủ lâu để queue/list wrap nhiều vòng.
- Chạy `test_integration_full_demo` và hai example.
- Theo dõi stack guard/high-water mark.
- Clean build, compiler warning, `git diff --check`.
- Xác nhận kernel/port không include board hoặc ST vendor header.
- Xác nhận một project CMake ngoài repo có thể mang `kernel/` và `port/` vào.
- Cập nhật docs theo đúng implementation vừa tự viết.

**Yêu cầu hoàn thành:** Public API, status code, test và tính năng tương thích
project hiện tại; không mất tính năng nào nêu tại mục 2.

## 5. Thứ tự đưa bộ test hiện tại trở lại

Các test nhỏ tự viết được dùng ở giai đoạn đầu. Khi API đã đủ, lần lượt đưa bộ
regression hiện tại trở lại:

```text
initial stack
  -> test_initial_stack
  -> test_first_task
  -> test_scheduler_cooperative
  -> test_scheduler_preemptive (round-robin trước)
  -> test_task_delay
  -> test_scheduler_priority
  -> test_stack_diagnostics
  -> test_semaphore_basic
  -> test_semaphore_blocking
  -> test_queue_fifo
  -> test_queue_blocking_receive
  -> test_queue_blocking_send
  -> test_scheduler_ready_list_priority
  -> test_scheduler_ready_list_blocking
  -> test_semaphore_isr
  -> test_queue_isr
  -> test_full_system_stress
  -> test_integration_full_demo
```

Sau mỗi giai đoạn phải chạy lại toàn bộ test đã pass trước đó. Không chỉ chạy
test mới nhất.

## 6. Những điểm cố ý viết rồi thay thế

| Bản đơn giản ban đầu | Được thay bằng | Lý do chỉ thay sau khi đã gặp vấn đề |
| --- | --- | --- |
| Chuyển cứng A/B | `task_table[]` | Thấy rõ hard-code không mở rộng |
| Round-robin table | Preemption bằng SysTick | Thấy rõ cooperative phụ thuộc task tự yield |
| Quét table chọn READY | Ready list theo priority | Đã có state/priority thật để thấy chi phí quét |
| `waiting_object` + quét task | IPC wait list | Đã có semaphore/queue thật để thấy coupling và lặp code |
| Logic block/wake riêng lẻ | Generic task block/make-ready helper | Chỉ trừu tượng hóa sau khi có ít nhất hai use case |

Bảng này là phần quan trọng nhất của roadmap: code tạm không phải code bỏ đi vô
nghĩa. Nó tạo ra bằng chứng để hiểu tại sao kiến trúc cuối cùng có hình dạng như
hiện tại.

## 7. Điều kiện được chuyển sang giai đoạn kế tiếp

Không chỉ cần “LED đã nháy”. Trước khi đi tiếp, phải trả lời được:

1. Phiên bản hiện tại đang giải quyết vấn đề gì?
2. Dữ liệu nào thuộc CPU, kernel, task hoặc IPC object?
3. Invariant nào đúng trước và sau mỗi thao tác?
4. Nếu interrupt chen vào giữa thao tác thì state có bị dở dang không?
5. Hạn chế cụ thể nào buộc phải thêm cơ chế ở giai đoạn tiếp theo?
6. Test nào sẽ fail nếu bỏ một bước quan trọng vừa viết?

Nếu chưa trả lời được câu 5 thì chưa nên refactor. Mục tiêu không phải đi nhanh
tới source cuối, mà là tự tái khám phá từng lý do khiến source cuối cần tồn tại.
