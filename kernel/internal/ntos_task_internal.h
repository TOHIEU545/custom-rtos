#ifndef NTOS_TASK_INTERNAL_H
#define NTOS_TASK_INTERNAL_H
#include <stddef.h>
#include <stdint.h>
#include <ntos/ntos_types.h>
#include "ntos_list_internal.h"
typedef enum { NTOS_TASK_UNUSED = 0, NTOS_TASK_READY, NTOS_TASK_RUNNING, NTOS_TASK_BLOCKED } ntos_task_state_t;
struct ntos_tcb {
    uint32_t *stack_pointer;
    uint32_t *stack_low;
    uint32_t *stack_high;
    size_t stack_words;
    ntos_task_entry_t entry;
    void *argument;
    const char *name;
    ntos_priority_t priority;
    ntos_task_state_t state;
    ntos_tick_t wake_tick;
    ntos_list_item_t ready_item;
    ntos_list_item_t wait_item;
};
#endif
