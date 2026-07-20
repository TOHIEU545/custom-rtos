#ifndef SCHEDULER_DEMO_H
#define SCHEDULER_DEMO_H
/**
 * @brief Demo fixed-priority scheduling: static task creation, preemption,
 *        ntos_delay(). Call scheduler_demo_init() after ntos_init(), then
 *        scheduler_demo_start() to hand control to the kernel (never returns).
 */

/** @brief Tạo task và trạng thái nội bộ của demo. */
void scheduler_demo_init(void);

/** @brief Chạy scheduler bắt đầu từ task priority cao nhất. Không return. */
void scheduler_demo_start(void);

#endif
