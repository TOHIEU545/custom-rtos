#ifndef IPC_DEMO_H
#define IPC_DEMO_H
/**
 * @brief Demo semaphore + queue + ISR-safe API + task block/wake. Call
 *        ipc_demo_init() after ntos_init(), then ipc_demo_start() to hand
 *        control to the kernel (never returns).
 */

/** @brief Tạo semaphore, queue, task và trạng thái nội bộ của demo. */
void ipc_demo_init(void);

/** @brief Chạy scheduler bắt đầu từ task priority cao nhất. Không return. */
void ipc_demo_start(void);

#endif
