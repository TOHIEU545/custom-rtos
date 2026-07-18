#ifndef NTOS_TASK_INTERNAL_H
#define NTOS_TASK_INTERNAL_H

/**
 * @file ntos_task_internal.h
 * @brief Header nội bộ kernel cho task - re-export type công khai, khai báo
 *        các transition BLOCKED<->READY dùng chung giữa ntos_sem.c/ntos_queue.c.
 */

#include <ntos/ntos_task.h>

/**
 * @brief Đưa current task vào BLOCKED, gắn lý do block, và chèn wait_item của
 *        nó vào wait_list chỉ định (waiting_tasks của semaphore, hoặc
 *        waiting_receivers/waiting_senders của queue).
 * @param wait_list Wait list đích, do caller (semaphore/queue) sở hữu.
 * @param reason Lý do block - phải khác NTOS_BLOCK_NONE/NTOS_BLOCK_DELAY
 *        (hai giá trị đó dành cho ntos_delay(), không dùng ở đây).
 * @note Gọi trong critical section (PRIMASK) của caller - không tự request
 *       PendSV, caller tự làm sau khi rời critical section (giống ntos_delay()).
 */
void ntos_task_block_current(ntos_list_t *wait_list, ntos_block_reason_t reason);

/**
 * @brief Gỡ task khỏi wait list hiện tại, reset block_reason, chuyển READY và
 *        chèn ready_item vào ready_lists[task->priority].
 * @param task Task đang BLOCKED vì IPC (block_reason != NONE/DELAY), lấy từ
 *        wait list bằng waiter-selection của caller.
 * @note Không copy/handoff dữ liệu gì - chỉ chuyển state/list membership.
 *       Dùng chung cho ntos_sem_give()/give_from_isr() và
 *       ntos_queue_send()/receive()/send_from_isr().
 */
void ntos_task_make_ready(ntos_tcb_t *task);

#endif
