# NTOS Architecture

## 1. Overview

NTOS là một RTOS nhỏ mang tính học tập cho **STM32F103 / ARM Cortex-M3**.

Kiến trúc được tách thành các khối chính để phân biệt rõ logic RTOS, phần phụ thuộc CPU
và phần phụ thuộc platform.

| Khối | Vai trò |
| --- | --- |
| **Kernel** | Quản lý task, scheduler, delay và IPC |
| **Cortex-M3 Port** | Xử lý context switch, SysTick, SVC và PendSV |
| **BSP** | Khởi tạo board và truy cập peripheral STM32F103 |
| **CMSIS** | Cung cấp định nghĩa Cortex-M3 core và STM32F10x device |

## 2. High-Level Architecture

```text
+------------------------------------------------------------------+
|                      APPLICATION / EXAMPLES                      |
|                                                                  |
|              Scheduler Demo · IPC Demo · User Tasks              |
+------------------------------------------------------------------+

+-------------------------------+  +-------------------------------+
|          NTOS RTOS            |  |       PLATFORM SUPPORT        |
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
|  |     CORTEX-M3 PORT      |  |  |  |         CMSIS           |  |
|  |                         |  |  |  |                         |  |
|  | Context Switch · SysTick|  |  |  | Cortex-M3 Core          |  |
|  | SVC · PendSV · PSP/MSP  |  |  |  | STM32F10x Device       |  |
|  +-------------------------+  |  |  +-------------------------+  |
+-------------------------------+  +-------------------------------+

+------------------------------------------------------------------+
|                         HARDWARE LAYER                           |
|                                                                  |
|       STM32F103 · ARM Cortex-M3 · NVIC · SysTick · Flash         |
|                 SRAM · GPIO · USART · EXTI                       |
+------------------------------------------------------------------+
```

Application sử dụng public API của NTOS để làm việc với kernel, đồng thời có thể sử dụng
BSP để truy cập phần cứng.

Kernel không phụ thuộc BSP. Phần phụ thuộc kiến trúc CPU được tách vào
`port/cortex_m3/`, còn phần phụ thuộc STM32F103 nằm trong `bsp/stm32f10x/`.

Cortex-M3 Port và BSP đều sử dụng CMSIS: port dùng các định nghĩa Cortex-M3 core,
còn BSP dùng các định nghĩa STM32F10x device.

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

Application truy cập kernel thông qua public API trong `include/ntos/`.
Các chi tiết nội bộ nằm trong `kernel/internal/`.

### Cortex-M3 Port

`port/cortex_m3/` chứa phần phụ thuộc kiến trúc ARM Cortex-M3.

| Cơ chế | Vai trò |
| --- | --- |
| **SVC** | Khởi động task đầu tiên |
| **PendSV** | Thực hiện context switch |
| **SysTick** | Cung cấp system tick cho kernel |
| **PSP / MSP** | Tách task stack và exception stack |

Chi tiết xem [Context Switch](context_switch.md).

### BSP

`bsp/stm32f10x/` chứa phần phụ thuộc STM32F103.

| Thành phần | Vai trò |
| --- | --- |
| Board / Clock | Khởi tạo board và system clock |
| GPIO / UART / EXTI | Truy cập peripheral |
| Startup | Reset flow và vector table |
| Linker script | Bố trí memory và section |

BSP độc lập với kernel và được application sử dụng trực tiếp khi cần truy cập phần cứng.

### CMSIS

`cmsis/` cung cấp các định nghĩa chuẩn cho Cortex-M3 core và STM32F10x device.

| Consumer | CMSIS usage |
| --- | --- |
| **Cortex-M3 Port** | Core definitions: NVIC, SysTick, SCB và CPU intrinsics |
| **BSP** | Device definitions: register và peripheral STM32F10x |

CMSIS không phải driver layer; nó cung cấp các định nghĩa mà port và BSP sử dụng để làm
việc với CPU core và peripheral.

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
