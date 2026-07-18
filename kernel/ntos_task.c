#include <ntos/ntos.h>
#include <ntos/ntos_config_default.h>
#include <ntos/ntos_task.h>

#include "ntos_assert_internal.h"
#include "ntos_kernel_internal.h"
#include "ntos_list_internal.h"
#include "ntos_port.h"

#include <stdint.h>

#define NTOS_TASK_FRAME_WORDS ((size_t)16U) /* 8 word hardware frame + 8 word R4-R11 */
#define NTOS_TASK_INITIAL_XPSR 0x01000000U  /* EPSR.T set - Cortex-M chi co Thumb state */

/* Gioi han cua ntos_delay(): phep so sanh tick wraparound-safe (ntos_tick_reached(),
 * Chang 7) dung signed subtraction nen chi dung voi do lech <= INT32_MAX tick. */
#define NTOS_DELAY_MAX_TICKS ((ntos_tick_t)INT32_MAX)

static void task_return_error(void)
{
    for (;;)
    {
    }
}

ntos_status_t ntos_task_create_static(ntos_tcb_t *tcb, const char *name, ntos_task_entry_t entry,
                                      void *argument, uint32_t *stack, size_t stack_words,
                                      ntos_priority_t priority)
{
    if ((tcb == NULL) || (stack == NULL) || (entry == NULL))
    {
        return NTOS_ERR_INVALID_ARGUMENT;
    }
    if (priority >= NTOS_MAX_PRIORITIES)
    {
        return NTOS_ERR_INVALID_PRIORITY;
    }
    if (stack_words < NTOS_TASK_FRAME_WORDS)
    {
        return NTOS_ERR_INVALID_ARGUMENT;
    }
    if (((uintptr_t)stack & 0x7U) != 0U)
    {
        return NTOS_ERR_INVALID_ARGUMENT;
    }
    if ((stack_words & 0x1U) != 0U)
    {
        return NTOS_ERR_INVALID_ARGUMENT;
    }
    if (tcb->state != NTOS_TASK_UNUSED)
    {
        return NTOS_ERR_INVALID_STATE;
    }

#if NTOS_ENABLE_STACK_FILL
    /* Fill 0xA5 toan bo stack de sau nay do duoc high-water mark. */
    for (size_t i = 0; i < stack_words; ++i)
    {
        stack[i] = 0xA5A5A5A5U;
    }
#endif

    /* Dựng initial frame giả, coi task như vừa bị interrupt lần đầu. */
    uint32_t *sp = stack + stack_words;

    *(--sp) = NTOS_TASK_INITIAL_XPSR;
    *(--sp) = (uint32_t)entry;             /* PC */
    *(--sp) = (uint32_t)task_return_error; /* LR */
    *(--sp) = 0x0000000CU;                 /* R12 */
    *(--sp) = 0x00000003U;                 /* R3 */
    *(--sp) = 0x00000002U;                 /* R2 */
    *(--sp) = 0x00000001U;                 /* R1 */
    *(--sp) = (uint32_t)argument;          /* R0 */
    *(--sp) = 0x0000000BU;                 /* R11 */
    *(--sp) = 0x0000000AU;                 /* R10 */
    *(--sp) = 0x00000009U;                 /* R9 */
    *(--sp) = 0x00000008U;                 /* R8 */
    *(--sp) = 0x00000007U;                 /* R7 */
    *(--sp) = 0x00000006U;                 /* R6 */
    *(--sp) = 0x00000005U;                 /* R5 */
    *(--sp) = 0x00000004U;                 /* R4 */

    tcb->stack_pointer = sp;
    tcb->stack_low = stack;
    tcb->stack_high = stack + stack_words;
    tcb->stack_words = stack_words;
    tcb->entry = entry;
    tcb->argument = argument;
    tcb->name = name;
    tcb->priority = priority;
    tcb->state = NTOS_TASK_READY;
    tcb->wake_tick = 0U; /* Chi co y nghia khi state == BLOCKED do ntos_delay() (Chang 7). */
    ntos_list_item_init(&tcb->ready_item, tcb);

    /* ntos_scheduler_register_task() (kernel/ntos_kernel.c) vua dang ky vao
     * task_table[] (con dung cho tick handler quet wake, Chang 7) vua chen
     * ready_item vao ready_lists[priority] (Chang 8). */
    ntos_scheduler_register_task(tcb);

    return NTOS_OK;
}

void ntos_yield(void)
{
    ntos_port_request_context_switch();
}

void ntos_delay(ntos_tick_t ticks)
{
    ntos_irq_state_t irq_state;

    if (ticks == 0U)
    {
        /* Quy uoc Chang 7: delay(0) tuong duong yield, khong block. */
        ntos_yield();
        return;
    }

    NTOS_ASSERT(ticks <= NTOS_DELAY_MAX_TICKS);
    NTOS_ASSERT(ntos_current_tcb != NULL);
    NTOS_ASSERT(ntos_current_tcb->state == NTOS_TASK_RUNNING); /* chi goi tu task context */
    NTOS_ASSERT(ntos_current_tcb != &ntos_idle_task_tcb);      /* idle khong duoc block */

    /* Critical section ngan: chi bao ve wake_tick + state + ready_item cua
     * chinh task hien tai khoi bi SysTick xen vao giua chung (neu PendSV chay
     * truoc khi state kip chuyen BLOCKED, scheduler se demote nham ve READY
     * thay vi giu BLOCKED - mat delay). Khong giu IRQ tat qua luc request
     * context switch. */
    irq_state = ntos_port_irq_save();

    ntos_current_tcb->wake_tick = ntos_tick_get() + ticks;
    ntos_current_tcb->state = NTOS_TASK_BLOCKED;
    /* BLOCKED task khong duoc nam trong ready list (Chang 8) - ntos_tick_handler()
     * se ntos_list_push_back() lai khi wake dung han. */
    ntos_list_remove(&ntos_current_tcb->ready_item);

    /* Debug check (muc 11): BLOCKED task tuyet doi khong duoc con container -
     * neu con nghia la ntos_list_remove() that bai hoac bi goi sai thu tu. */
    NTOS_ASSERT(ntos_current_tcb->ready_item.container == NULL);

    ntos_port_irq_restore(irq_state);

    ntos_port_request_context_switch();
}
