#ifndef RTOS_PORT_H
#define RTOS_PORT_H

/**
 * @file rtos_port.h
 * @brief API port Cortex-M3: start scheduler, request context switch, critical section.
 */

#include "rtos.h"

#include <stddef.h>
#include <stdint.h>

/** Hardware exception frame plus software-saved R4-R11. */
#define RTOS_PORT_INITIAL_FRAME_WORDS ((size_t)16U)

/** @brief Kiểu lưu trạng thái PRIMASK, dùng cho rtos_port_irq_save()/rtos_port_irq_restore(). */
typedef uint32_t rtos_irq_state_t;

/** Build the initial Cortex-M3 context frame at the top of a task stack. */
uint32_t *rtos_port_stack_init(uint32_t *stack, size_t stack_words,
                               rtos_task_entry_t entry, void *argument);

/**
 * @brief Chạy task đầu tiên (rtos_current_tcb) bằng SVC; không bao giờ return.
 * @note Yêu cầu rtos_current_tcb đã được gán trước khi gọi (xem kernel/src/core.c).
 */
void rtos_port_start_scheduler(void) __attribute__((noreturn));

/**
 * @brief Pend PendSV để yêu cầu context switch.
 * @note Dùng chung bởi rtos_yield() và rtos_tick_handler() - nơi duy nhất set PENDSVSET.
 */
void rtos_port_request_context_switch(void);

/**
 * @brief Set priority PendSV/SysTick về thấp nhất rồi cấu hình + start SysTick ở RTOS_TICK_RATE_HZ.
 * @note Gọi một lần trong rtos_start(), trước khi bật interrupt.
 */
void rtos_port_start_tick(uint32_t cpu_clock_hz);

/** Stop execution after a fatal kernel invariant failure. */
void rtos_port_panic(void) __attribute__((noreturn));

/**
 * @brief Vào critical section: lưu PRIMASK hiện tại rồi tắt interrupt.
 * @return Trạng thái PRIMASK trước đó, truyền lại cho rtos_port_irq_restore().
 */
rtos_irq_state_t rtos_port_irq_save(void);

/**
 * @brief Ra critical section: khôi phục PRIMASK về trạng thái đã lưu.
 * @param state Giá trị trả về từ rtos_port_irq_save().
 */
void rtos_port_irq_restore(rtos_irq_state_t state);

#endif
