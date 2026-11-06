# Known Limitations

MyRTOS là một RTOS nhỏ mang tính học tập, nên cố ý giữ phạm vi hẹp để ưu tiên
tính đơn giản, khả năng đọc source và hiểu cơ chế bên trong.

| Area | Limitation |
| --- | --- |
| Memory | Toàn bộ TCB, stack, semaphore, queue đều cấp phát tĩnh do caller cung cấp; không dùng heap ở bất kỳ đâu trong kernel/port. |
| Memory | Số task tối đa cố định (`RTOS_MAX_TASKS`, hiện là 9) và số mức priority cố định (`RTOS_MAX_PRIORITIES`, hiện là 8), khai báo tại compile-time, đổi phải build lại. |
| Tasks | Không có API xoá, suspend, hay resume task — task tồn tại tĩnh từ lúc tạo tới hết vòng đời chương trình, chỉ state (READY/RUNNING/BLOCKED) thay đổi. |
| Tasks | Priority được gán cố định lúc tạo task, không có API đổi priority sau đó. |
| Scheduler | Chỉ có một policy: fixed-priority preemptive; không có priority inheritance/priority ceiling, nên một task priority thấp giữ tài nguyên vẫn có thể chặn gián tiếp task priority cao (priority inversion không được xử lý). |
| Scheduler | Task hết delay được phát hiện bằng cách quét tuyến tính toàn bộ `task_table[]` mỗi tick, không dùng delayed list riêng — phù hợp với số task nhỏ, không tối ưu cho số task lớn. |
| Synchronization | Chỉ có hai cơ chế IPC: semaphore đếm và queue tĩnh; không có mutex, event flags/group, hay condition variable. |
| Synchronization | `timeout` của `rtos_sem_take()`/`rtos_queue_send()`/`rtos_queue_receive()` chỉ phân biệt "không chờ" (`RTOS_NO_WAIT`) và "chờ vô hạn"; mọi giá trị khác 0 đều bị xử lý như chờ vô hạn, không có timed blocking hữu hạn thật sự. |
| Timing | Tick rate cố định (`RTOS_TICK_RATE_HZ`) tại compile-time; SysTick luôn chạy, không có tickless idle. |
| Timing | Một lần `rtos_delay()` tối đa `INT32_MAX` tick, do phép so sánh tick wraparound-safe dùng signed subtraction. |
| Portability | Chỉ hỗ trợ ARM Cortex-M3 (`port/ARM/cortex-m3/`) và board STM32F103C8T6 (`boards/STM32F103C8T6/`); không có port cho CPU/board khác. |
| Portability | Chỉ single-core; không hỗ trợ SMP/multi-core. |

Các giới hạn này là giới hạn có chủ đích của phiên bản hiện tại, không phải
mô tả đầy đủ của một RTOS production-grade.
