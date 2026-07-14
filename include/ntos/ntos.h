#ifndef NTOS_H
#define NTOS_H

/**
 * @file ntos.h
 * @brief API tổng quát: khởi tạo kernel, start scheduler, đọc tick hệ thống.
 */

#include <ntos/ntos_queue.h>
#include <ntos/ntos_sem.h>
#include <ntos/ntos_task.h>

/** @brief Khởi tạo kernel trước khi tạo task. @note TODO(stage 2+). */
void ntos_init(void);

/**
 * @brief Chạy first_task bằng SVC/PSP; không bao giờ return.
 * @param first_task TCB đã tạo qua ntos_task_create_static(), state phải là NTOS_TASK_READY.
 */
void ntos_start(ntos_tcb_t *first_task) __attribute__((noreturn));

/** @brief Đọc số tick hệ thống hiện tại. @note CHƯA IMPLEMENT (stage 5+) - luôn trả 0. */
ntos_tick_t ntos_tick_get(void);
#endif
