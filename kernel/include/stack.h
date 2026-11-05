#ifndef RTOS_STACK_H
#define RTOS_STACK_H

#include "task.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define RTOS_STACK_FILL_PATTERN ((uint32_t)0xA5A5A5A5U)

typedef struct rtos_task_stack_info
{
    size_t stack_words;
    size_t high_water_mark_words;
    size_t untouched_words;
    bool guard_intact;
} rtos_task_stack_info_t;

rtos_status_t rtos_task_stack_get_info(const rtos_tcb_t *tcb,
                                       rtos_task_stack_info_t *info);
bool rtos_task_stack_guard_ok(const rtos_tcb_t *tcb);

#endif
