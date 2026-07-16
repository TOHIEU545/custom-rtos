#include <ntos/ntos.h>

#include <ntos/ntos_config_default.h>

#include "ntos_assert_internal.h"
#include "ntos_kernel_internal.h"

#include "ntos_port.h"
#include <stddef.h>
#include <stdint.h>

struct ntos_tcb *volatile ntos_current_tcb = NULL;

static ntos_tcb_t *task_table[NTOS_MAX_TASKS];
static size_t task_count = 0U;
static size_t current_task_index = 0U;

void ntos_init(void)
{ /* TODO(stage 2+). */
}

void ntos_start(ntos_tcb_t *first_task_tcb)
{
    size_t index;

    NTOS_ASSERT(ntos_current_tcb == NULL);
    NTOS_ASSERT(first_task_tcb != NULL);
    NTOS_ASSERT(first_task_tcb->state == NTOS_TASK_READY);
    NTOS_ASSERT(first_task_tcb->stack_pointer != NULL);
    NTOS_ASSERT(first_task_tcb->stack_pointer >= first_task_tcb->stack_low);
    NTOS_ASSERT(first_task_tcb->stack_pointer < first_task_tcb->stack_high);
    NTOS_ASSERT(((uintptr_t)first_task_tcb->stack_pointer & 0x7U) == 0U);

    for (index = 0U; index < task_count; ++index)
    {
        if (task_table[index] == first_task_tcb)
        {
            break;
        }
    }
    NTOS_ASSERT(index < task_count); /* first_task phai da duoc dang ky qua ntos_task_create_static() */

    current_task_index = index;
    first_task_tcb->state = NTOS_TASK_RUNNING;
    ntos_current_tcb = first_task_tcb;

    ntos_port_start_scheduler();
}

ntos_tick_t ntos_tick_get(void)
{
    return 0U; /* TODO(stage 5+). */
}

void ntos_scheduler_register_task(ntos_tcb_t *tcb)
{
    NTOS_ASSERT(tcb != NULL);
    NTOS_ASSERT(task_count < NTOS_MAX_TASKS);

    task_table[task_count] = tcb;
    task_count++;
}

void ntos_scheduler_select_next(void)
{
    size_t next_index;
    size_t i;

    NTOS_ASSERT(ntos_current_tcb != NULL);
    NTOS_ASSERT(task_count > 0U);

    if (ntos_current_tcb->state == NTOS_TASK_RUNNING)
    {
        ntos_current_tcb->state = NTOS_TASK_READY;
    }

    next_index = current_task_index;
    for (i = 0U; i < task_count; ++i)
    {
        next_index = (next_index + 1U) % task_count;
        if (task_table[next_index]->state == NTOS_TASK_READY)
        {
            break;
        }
    }

    current_task_index = next_index;
    ntos_current_tcb = task_table[current_task_index];
    ntos_current_tcb->state = NTOS_TASK_RUNNING;
}
