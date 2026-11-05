#ifndef RTOS_TASK_H
#define RTOS_TASK_H

#include "core.h"
#include "list.h"

#include <stddef.h>
#include <stdint.h>

typedef enum
{
    RTOS_TASK_UNUSED = 0,
    RTOS_TASK_READY,
    RTOS_TASK_RUNNING,
    RTOS_TASK_BLOCKED
} rtos_task_state_t;

typedef enum
{
    RTOS_BLOCK_NONE = 0,
    RTOS_BLOCK_DELAY,
    RTOS_BLOCK_SEMAPHORE,
    RTOS_BLOCK_QUEUE_SEND,
    RTOS_BLOCK_QUEUE_RECEIVE
} rtos_block_reason_t;

typedef struct rtos_tcb
{
    uint32_t *stack_pointer;
    uint32_t *stack_low;
    uint32_t *stack_high;
    size_t stack_words;
    rtos_task_entry_t entry;
    void *argument;
    const char *name;
    rtos_priority_t priority;
    rtos_task_state_t state;
    rtos_block_reason_t block_reason;
    rtos_tick_t wake_tick;
    rtos_list_item_t ready_item;
    rtos_list_item_t wait_item;
} rtos_tcb_t;

rtos_status_t rtos_task_create_static(rtos_tcb_t *tcb, const char *name,
                                      rtos_task_entry_t entry, void *argument,
                                      uint32_t *stack, size_t stack_words,
                                      rtos_priority_t priority);
void rtos_yield(void);
void rtos_delay(rtos_tick_t ticks);

#endif
