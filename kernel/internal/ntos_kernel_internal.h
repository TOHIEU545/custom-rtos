#ifndef NTOS_KERNEL_INTERNAL_H
#define NTOS_KERNEL_INTERNAL_H

/**
 * @file ntos_kernel_internal.h
 * @brief Symbol nội bộ kernel dùng chung giữa kernel/ và port/ (current TCB, scheduler, tick).
 */

#include "ntos_task_internal.h"

/** @brief TCB của task đang chạy hiện tại; NULL trước khi ntos_start() gán. Định nghĩa tại kernel/ntos_kernel.c. */
extern struct ntos_tcb *volatile ntos_current_tcb;

/** @brief Đăng ký tcb vào task_table của scheduler. Gọi từ ntos_task_create_static(). */
void ntos_scheduler_register_task(ntos_tcb_t *tcb);

/** @brief Chọn task kế tiếp cho PendSV context switch (round-robin, Chặng 4). */
void ntos_scheduler_select_next(void);

/** @brief CHƯA IMPLEMENT (TODO stage 5) - xử lý tick từ SysTick. */
void ntos_tick_handler(void);

#endif
