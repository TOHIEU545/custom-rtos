#include <ntos/ntos.h>

#include <ntos/ntos_config_default.h>

#include "ntos_assert_internal.h"
#include "ntos_kernel_internal.h"

#include "ntos_port.h"
#include <stddef.h>
#include <stdint.h>

struct ntos_tcb *volatile ntos_current_tcb = NULL;
volatile bool ntos_scheduler_running = false;
static volatile ntos_tick_t ntos_tick_count = 0U;

static ntos_tcb_t *task_table[NTOS_MAX_TASKS];
static size_t task_count = 0U;
static size_t current_task_index = 0U;

ntos_tcb_t ntos_idle_task_tcb;
static _Alignas(8) uint32_t ntos_idle_task_stack[NTOS_IDLE_STACK_WORDS];

/* Idle task Chang 6: khong lam gi ca, khong block, khong goi semaphore/queue.
 * Priority 0 (thap nhat) - chi duoc chon khi khong con task application nao
 * priority cao hon dang READY (xem quy uoc round-robin cung priority da
 * quyet dinh trong ntos_scheduler_select_next()). */
static void ntos_idle_task(void *argument)
{
    (void)argument;

    for (;;)
    {
    }
}

void ntos_init(void)
{
    ntos_status_t status = ntos_task_create_static(&ntos_idle_task_tcb, "idle", ntos_idle_task,
        NULL, ntos_idle_task_stack, NTOS_IDLE_STACK_WORDS, 0U);

    NTOS_ASSERT(status == NTOS_OK);
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

    /* Priority PendSV/SysTick va SysTick timer phai san sang truoc khi bat interrupt.
     * ntos_scheduler_running KHONG duoc dat true o day - SVC_Handler (port) tu dat
     * true ngay sau khi PSP cua first task hop le, de loai bo hoan toan kha nang
     * SysTick pend PendSV truoc khi PSP san sang (vd. neu SysTick xen vao giua
     * __enable_irq() va lenh svc trong ntos_port_start_scheduler()). */
    ntos_port_start_tick();

    ntos_port_start_scheduler();
}

ntos_tick_t ntos_tick_get(void)
{
    return ntos_tick_count;
}

void ntos_tick_handler(void)
{
    ntos_tick_count++;

    if (ntos_scheduler_running)
    {
        ntos_port_request_context_switch();
    }
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
    ntos_priority_t highest_ready_priority;
    bool found_ready;

    NTOS_ASSERT(ntos_current_tcb != NULL);
    NTOS_ASSERT(task_count > 0U);

    if (ntos_current_tcb->state == NTOS_TASK_RUNNING)
    {
        ntos_current_tcb->state = NTOS_TASK_READY;
    }

    /* Pass 1 (O(n)): tim priority cao nhat trong so cac task dang READY. Task
     * hien tai da duoc demote ve READY o tren nen khong can xet rieng RUNNING. */
    highest_ready_priority = 0U;
    found_ready = false;
    for (i = 0U; i < task_count; ++i)
    {
        if (task_table[i]->state == NTOS_TASK_READY)
        {
            if (!found_ready || (task_table[i]->priority > highest_ready_priority))
            {
                highest_ready_priority = task_table[i]->priority;
            }
            found_ready = true;
        }
    }
    /* Phai luon co it nhat 1 task READY (chinh current vua duoc demote o tren). */
    NTOS_ASSERT(found_ready);

    /* Pass 2 (O(n)): round-robin tu current_task_index, CHI chon task READY co
     * dung highest_ready_priority - task priority thap hon du dang READY van bi
     * bo qua (starvation hop le cua fixed-priority scheduler, khong phai bug). */
    next_index = current_task_index;
    for (i = 0U; i < task_count; ++i)
    {
        next_index = (next_index + 1U) % task_count;
        if ((task_table[next_index]->state == NTOS_TASK_READY) &&
            (task_table[next_index]->priority == highest_ready_priority))
        {
            break;
        }
    }

    /* Invariant Chang 6: scheduler tuyet doi khong duoc chon task UNUSED/BLOCKED,
     * va phai la task co priority cao nhat dang READY. Neu assert nay no nghia la
     * khong tim thay task nao khop - loi logic kernel that su. */
    NTOS_ASSERT(task_table[next_index]->state == NTOS_TASK_READY);
    NTOS_ASSERT(task_table[next_index]->priority == highest_ready_priority);

    current_task_index = next_index;
    ntos_current_tcb = task_table[current_task_index];
    ntos_current_tcb->state = NTOS_TASK_RUNNING;
}
