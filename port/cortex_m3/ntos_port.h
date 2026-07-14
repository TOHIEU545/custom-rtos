#ifndef NTOS_PORT_H
#define NTOS_PORT_H

/**
 * @file ntos_port.h
 * @brief API port Cortex-M3: start scheduler, request context switch, critical section.
 */

#include <stdint.h>

/** @brief Kiểu lưu trạng thái PRIMASK, dùng cho ntos_port_irq_save()/ntos_port_irq_restore(). */
typedef uint32_t ntos_irq_state_t;

/**
 * @brief Chạy task đầu tiên (ntos_current_tcb) bằng SVC; không bao giờ return.
 * @note Yêu cầu ntos_current_tcb đã được gán trước khi gọi (xem kernel/ntos_kernel.c).
 */
void ntos_port_start_scheduler(void) __attribute__((noreturn));

/**
 * @brief Pend PendSV để yêu cầu context switch.
 * @note CHƯA có tác dụng thật - PendSV_Handler còn là stub (TODO stage 4).
 */
void ntos_port_request_context_switch(void);

/**
 * @brief Vào critical section: lưu PRIMASK hiện tại rồi tắt interrupt.
 * @return Trạng thái PRIMASK trước đó, truyền lại cho ntos_port_irq_restore().
 */
ntos_irq_state_t ntos_port_irq_save(void);

/**
 * @brief Ra critical section: khôi phục PRIMASK về trạng thái đã lưu.
 * @param state Giá trị trả về từ ntos_port_irq_save().
 */
void ntos_port_irq_restore(ntos_irq_state_t state);

#endif
