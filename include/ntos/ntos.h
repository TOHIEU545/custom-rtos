#ifndef NTOS_H
#define NTOS_H

/**
 * @file ntos.h
 * @brief API tổng quát: khởi tạo kernel, start scheduler, đọc tick hệ thống.
 */

#include <ntos/ntos_queue.h>
#include <ntos/ntos_sem.h>
#include <ntos/ntos_task.h>

/**
 * @brief Khởi tạo kernel trước khi tạo task application - tạo idle task (priority 0), tạo list.
 * @note Phải gọi trước ntos_task_create_static()/ntos_start().
 */
void ntos_init(void);

/**
 * @brief Chạy first_task bằng SVC/PSP; không bao giờ return.
 * @param first_task_tcb TCB đã tạo qua ntos_task_create_static(), state phải là NTOS_TASK_READY.
 */
void ntos_start(ntos_tcb_t *first_task_tcb) __attribute__((noreturn));

/** @brief Đọc số tick hệ thống hiện tại, tăng mỗi lần SysTick fire. */
ntos_tick_t ntos_tick_get(void);
#endif
