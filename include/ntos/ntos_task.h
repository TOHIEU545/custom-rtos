#ifndef NTOS_TASK_H
#define NTOS_TASK_H
#include <stddef.h>
#include <stdint.h>
#include <ntos/ntos_status.h>
#include <ntos/ntos_types.h>
typedef struct ntos_tcb ntos_tcb_t;
ntos_status_t ntos_task_create_static(ntos_tcb_t *tcb, const char *name,
    ntos_task_entry_t entry, void *argument, uint32_t *stack,
    size_t stack_words, ntos_priority_t priority);
void ntos_yield(void);
void ntos_delay(ntos_tick_t ticks);
#endif
