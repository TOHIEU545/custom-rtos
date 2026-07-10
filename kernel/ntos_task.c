#include <ntos/ntos_task.h>
ntos_status_t ntos_task_create_static(ntos_tcb_t *tcb, const char *name,
    ntos_task_entry_t entry, void *argument, uint32_t *stack,
    size_t stack_words, ntos_priority_t priority)
{
    (void)tcb; (void)name; (void)entry; (void)argument;
    (void)stack; (void)stack_words; (void)priority;
    return NTOS_ERR_INTERNAL; /* TODO(stage 2). */
}
void ntos_yield(void) { /* TODO(stage 4). */ }
void ntos_delay(ntos_tick_t ticks) { (void)ticks; /* TODO(stage 7). */ }
