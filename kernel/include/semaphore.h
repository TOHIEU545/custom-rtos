#ifndef RTOS_SEMAPHORE_H
#define RTOS_SEMAPHORE_H

#include "core.h"
#include "list.h"

#include <stdbool.h>
#include <stdint.h>

typedef struct rtos_semaphore
{
    uint32_t count;
    uint32_t max_count;
    rtos_list_t waiting_tasks;
} rtos_semaphore_t;

void rtos_sem_init(rtos_semaphore_t *sem, uint32_t initial_count,
                   uint32_t max_count);
rtos_status_t rtos_sem_take(rtos_semaphore_t *sem, rtos_tick_t timeout);
rtos_status_t rtos_sem_give(rtos_semaphore_t *sem);
rtos_status_t rtos_sem_give_from_isr(rtos_semaphore_t *sem,
                                     bool *higher_priority_task_woken);

#endif
