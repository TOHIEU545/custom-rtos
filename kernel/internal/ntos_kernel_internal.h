#ifndef NTOS_KERNEL_INTERNAL_H
#define NTOS_KERNEL_INTERNAL_H

/**
 * @file ntos_kernel_internal.h
 * @brief Symbol nội bộ kernel dùng chung giữa kernel/ và port/ (current TCB, scheduler, tick).
 */

#include <stdbool.h>

#include "ntos_task_internal.h"

/** @brief TCB của task đang chạy hiện tại; NULL trước khi ntos_start() gán. Định nghĩa tại kernel/ntos_kernel.c. */
extern struct ntos_tcb *volatile ntos_current_tcb;

/**
 * @brief true khi PSP của first task đã hợp lệ (Chặng 5); trước đó SysTick không được request context switch.
 * @note Chỉ SVC_Handler (port/cortex_m3/ntos_port.c) được set true, ngay sau khi PSP hợp lệ -
 *       KHÔNG phải ntos_start(). Định nghĩa (khởi tạo false) tại kernel/ntos_kernel.c.
 */
extern volatile bool ntos_scheduler_running;

/**
 * @brief TCB của idle task do kernel tự tạo trong ntos_init() (Chặng 6), priority 0.
 * @note Export chỉ để test so sánh con trỏ (ntos_current_tcb == &ntos_idle_task_tcb).
 *       Định nghĩa tại kernel/ntos_kernel.c.
 */
extern ntos_tcb_t ntos_idle_task_tcb;

/** @brief Đăng ký tcb vào task_table của scheduler. Gọi từ ntos_task_create_static(). */
void ntos_scheduler_register_task(ntos_tcb_t *tcb);

/**
 * @brief Chọn task kế tiếp cho PendSV context switch: fixed-priority O(n), round-robin
 *        giữa các task cùng priority cao nhất đang READY (Chặng 6).
 */
void ntos_scheduler_select_next(void);

/** @brief Xử lý 1 tick từ SysTick_Handler: tăng ntos_tick_count, request PendSV nếu scheduler đang chạy (Chặng 5). */
void ntos_tick_handler(void);

#endif
