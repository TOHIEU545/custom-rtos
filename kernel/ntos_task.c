#include <ntos/ntos_config_default.h>
#include <ntos/ntos_task.h>

#define NTOS_TASK_FRAME_WORDS ((size_t)16U) /* 8 word hardware frame + 8 word R4-R11 */
#define NTOS_TASK_INITIAL_XPSR 0x01000000U  /* EPSR.T set - Cortex-M chi co Thumb state */

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

    return NTOS_OK;
}

void ntos_yield(void)
{ /* TODO(stage 4). */
}
void ntos_delay(ntos_tick_t ticks)
{
    (void)ticks; /* TODO(stage 7). */
}
