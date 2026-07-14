#include <ntos/ntos.h>

#include "ntos_assert_internal.h"
#include "ntos_kernel_internal.h"

#include "ntos_port.h"

#include <stdint.h>

struct ntos_tcb *volatile ntos_current_tcb = NULL;

void ntos_init(void)
{ /* TODO(stage 2+). */
}

void ntos_start(ntos_tcb_t *first_task)
{
    NTOS_ASSERT(ntos_current_tcb == NULL);
    NTOS_ASSERT(first_task != NULL);
    NTOS_ASSERT(first_task->state == NTOS_TASK_READY);
    NTOS_ASSERT(first_task->stack_pointer != NULL);
    NTOS_ASSERT(first_task->stack_pointer >= first_task->stack_low);
    NTOS_ASSERT(first_task->stack_pointer < first_task->stack_high);
    NTOS_ASSERT(((uintptr_t)first_task->stack_pointer & 0x7U) == 0U);

    first_task->state = NTOS_TASK_RUNNING;
    ntos_current_tcb = first_task;

    ntos_port_start_scheduler();
}

ntos_tick_t ntos_tick_get(void)
{
    return 0U; /* TODO(stage 5+). */
}
