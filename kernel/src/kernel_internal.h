#ifndef RTOS_KERNEL_INTERNAL_H
#define RTOS_KERNEL_INTERNAL_H

#include "rtos.h"

#include <stdbool.h>

void rtos_assert_failed(const char *file, int line, const char *expression);

#define RTOS_ASSERT(expr)                                                                          \
    do                                                                                             \
    {                                                                                              \
        if (!(expr))                                                                               \
        {                                                                                          \
            rtos_assert_failed(__FILE__, __LINE__, #expr);                                         \
        }                                                                                          \
    } while (0)

void rtos_task_block_current(rtos_list_t *wait_list, rtos_block_reason_t reason);
void rtos_task_make_ready(rtos_tcb_t *task);
rtos_tcb_t *rtos_waiter_select_highest(rtos_list_t *waiting_tasks);

extern struct rtos_tcb *volatile rtos_current_tcb;
extern volatile bool rtos_scheduler_running;
extern rtos_tcb_t rtos_idle_task_tcb;
extern rtos_list_t ready_lists[];

void rtos_scheduler_register_task(rtos_tcb_t *tcb);
void rtos_scheduler_select_next(void);
void rtos_tick_handler(void);
bool rtos_tick_reached(rtos_tick_t now, rtos_tick_t target);

#endif
