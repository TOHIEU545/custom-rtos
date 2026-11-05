# MyRTOS Architecture

## 1. Overview

MyRTOS là một RTOS nhỏ mang tính học tập cho **STM32F103 / ARM Cortex-M3**.

Kiến trúc được tách thành các khối chính để phân biệt rõ logic RTOS, phần phụ thuộc CPU
và phần phụ thuộc platform.

| Khối | Vai trò |
| --- | --- |
| **Kernel** | Quản lý task, scheduler, delay và IPC |
| **Cortex-M3 Port** | Xử lý context switch, SysTick, SVC và PendSV |
| **BSP** | Khởi tạo board và truy cập peripheral STM32F103 |
| **CMSIS-Core** | Định nghĩa Cortex-M3 chuẩn do Arm cung cấp, đặt cục bộ trong từng module |
| **ST device files** | Định nghĩa STM32F10x và system initialization do ST cung cấp |

## 2. High-Level Architecture

```text
+------------------------------------------------------------------+
|                      APPLICATION / EXAMPLES                      |
|                                                                  |
|              Scheduler Demo · IPC Demo · User Tasks              |
+------------------------------------------------------------------+

+-------------------------------+  +-------------------------------+
|          MyRTOS RTOS            |  |       PLATFORM SUPPORT        |
|                               |  |                               |
|  +-------------------------+  |  |  +-------------------------+  |
|  |         KERNEL          |  |  |  |          BSP            |  |
|  |                         |  |  |  |                         |  |
|  | Task · Scheduler        |  |  |  | Board · Clock           |  |
|  | Delay · Semaphore       |  |  |  | GPIO · UART · EXTI      |  |
|  | Queue · Stack           |  |  |  | Startup · Linker        |  |
|  +-------------------------+  |  |  +-------------------------+  |
|                               |  |                               |
|  +-------------------------+  |  |  +-------------------------+  |
|  |     CORTEX-M3 PORT      |  |  |  |    BOARD + VENDOR       |  |
|  |                         |  |  |  |                         |  |
|  | Context Switch · SysTick|  |  |  | CMSIS-Core · ST Device  |  |
|  | SVC · PendSV · PSP/MSP  |  |  |  | Startup · Linker        |  |
|  +-------------------------+  |  |  +-------------------------+  |
+-------------------------------+  +-------------------------------+

+------------------------------------------------------------------+
|                         HARDWARE LAYER                           |
|                                                                  |
|       STM32F103 · ARM Cortex-M3 · NVIC · SysTick · Flash         |
|                 SRAM · GPIO · USART · EXTI                       |
+------------------------------------------------------------------+
```

Application sử dụng public API của MyRTOS để làm việc với kernel, đồng thời có thể sử dụng
BSP để truy cập phần cứng.

Kernel không phụ thuộc BSP. Phần phụ thuộc kiến trúc CPU được tách vào
`port/ARM/cortex-m3/`, còn phần phụ thuộc STM32F103 nằm trong `boards/STM32F103/`.

Cortex-M3 port chỉ dùng CMSIS-Core nằm ngay trong port. BSP dùng bản CMSIS-Core
cục bộ của board và device files do ST cung cấp. Không module nào phụ thuộc một
thư mục vendor dùng chung ở root.

## 3. Components

### Kernel

`kernel/` chứa logic RTOS độc lập với phần cứng cụ thể.

| Chức năng chính |
| --- |
| Task management |
| Fixed-priority scheduler |
| Delay và system tick handling |
| Semaphore và queue |
| Ready list / wait list infrastructure |
| Stack diagnostics |

Application truy cập kernel thông qua public API trong `kernel/`.
Các chi tiết nội bộ nằm trong `kernel/`.

### Cortex-M3 Port

`port/ARM/cortex-m3/` chứa phần phụ thuộc kiến trúc ARM Cortex-M3.

| Cơ chế | Vai trò |
| --- | --- |
| **SVC** | Khởi động task đầu tiên |
| **PendSV** | Thực hiện context switch |
| **SysTick** | Cung cấp system tick cho kernel |
| **PSP / MSP** | Tách task stack và exception stack |

Port không include `stm32f10x.h`, `bsp.h` hay bất kỳ header nào từ `boards/`.
Tần số CPU được application truyền vào `rtos_start()`, nên port không phụ thuộc
biến `SystemCoreClock` của ST.

Chi tiết xem [Context Switch](context_switch.md).

### BSP

`boards/STM32F103/` chứa phần phụ thuộc STM32F103.

| Thành phần | Vai trò |
| --- | --- |
| Board / Clock | Khởi tạo board và system clock |
| GPIO / UART / EXTI | Truy cập peripheral |
| Startup | Reset flow và vector table |
| Linker script | Bố trí memory và section |

BSP độc lập với kernel và được application sử dụng trực tiếp khi cần truy cập phần cứng.

### Third-party source

Code bên thứ ba không trộn với code MyRTOS tự viết:

| Vị trí | Nguồn | Nội dung |
| --- | --- | --- |
| `port/ARM/cortex-m3/cmsis/` | Arm | CMSIS-Core tối thiểu cho Cortex-M3 port |
| `boards/STM32F103/cmsis/` | Arm | CMSIS-Core dùng bởi ST device header |
| `boards/STM32F103/vendor/ST/` | ST | `stm32f10x.h`, system header/source |

CMSIS-Core không phải driver layer. ST device files cung cấp register layout,
interrupt number và system clock implementation riêng cho STM32F10x.

## 4. Related Documentation

| Tài liệu | Nội dung |
| --- | --- |
| [Context Switch](context_switch.md) | SVC, PendSV, exception frame và context switching |
| [Scheduler](scheduler.md) | Fixed-priority scheduling và round-robin |
| [Task States](task_states.md) | READY, RUNNING, BLOCKED và state transition |
| [IPC](ipc.md) | Semaphore, queue và ISR interaction |
| [Testing](testing.md) | Test strategy và phạm vi regression test |
| [Known Limitations](known_limitations.md) | Các giới hạn thiết kế hiện tại |
| [CMake Build Flow](cmake_flow.md) | Cách các module được ghép thành firmware |
