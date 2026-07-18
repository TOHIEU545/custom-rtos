/**
 * @file ntos_task.c
 * @brief Tạo task tĩnh (dựng initial stack frame), yield, delay.
 */
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

/* Giới hạn của ntos_delay(): phép so sánh tick wraparound-safe (ntos_tick_reached())
 * dùng signed subtraction nên chỉ đúng với độ lệch <= INT32_MAX tick. */
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
    /* Fill 0xA5 toàn bộ stack để sau này đo được high-water mark. */
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
    tcb->block_reason = NTOS_BLOCK_NONE; /* Chỉ có ý nghĩa khi state == BLOCKED. */
    tcb->wake_tick = 0U; /* Chỉ có ý nghĩa khi block_reason == NTOS_BLOCK_DELAY. */
    ntos_list_item_init(&tcb->ready_item, tcb);
    ntos_list_item_init(&tcb->wait_item, tcb);

    /* ntos_scheduler_register_task() (kernel/ntos_kernel.c) vừa đăng ký vào
     * task_table[] (còn dùng cho tick handler quét wake) vừa chèn ready_item
     * vào ready_lists[priority]. */
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
        /* Quy ước: delay(0) tương đương yield, không block. */
        ntos_yield();
        return;
    }

    NTOS_ASSERT(ticks <= NTOS_DELAY_MAX_TICKS);
    NTOS_ASSERT(ntos_current_tcb != NULL);
    NTOS_ASSERT(ntos_current_tcb->state == NTOS_TASK_RUNNING); /* chỉ gọi từ task context */
    NTOS_ASSERT(ntos_current_tcb != &ntos_idle_task_tcb);      /* idle không được block */
    /* Task delay không được đồng thời chờ IPC nào - hai cơ chế block hoàn
     * toàn tách biệt (xem block_reason, include/ntos/ntos_task.h). */
    NTOS_ASSERT(ntos_current_tcb->wait_item.container == NULL);

    /* Critical section ngắn: chỉ bảo vệ wake_tick + state + ready_item của
     * chính task hiện tại khỏi bị SysTick xen vào giữa chừng (nếu PendSV chạy
     * trước khi state kịp chuyển BLOCKED, scheduler sẽ demote nhầm về READY
     * thay vì giữ BLOCKED - mất delay). Không giữ IRQ tắt qua lúc request
     * context switch. */
    irq_state = ntos_port_irq_save();

    ntos_current_tcb->wake_tick = ntos_tick_get() + ticks;
    ntos_current_tcb->state = NTOS_TASK_BLOCKED;
    ntos_current_tcb->block_reason = NTOS_BLOCK_DELAY;
    /* BLOCKED task không được nằm trong ready list - ntos_tick_handler()
     * sẽ ntos_list_push_back() lại khi wake đúng hạn. */
    ntos_list_remove(&ntos_current_tcb->ready_item);

    /* Assert BLOCKED task tuyệt đối không được còn container - nếu còn nghĩa
     * là ntos_list_remove() thất bại hoặc bị gọi sai thứ tự. */
    NTOS_ASSERT(ntos_current_tcb->ready_item.container == NULL);

    ntos_port_irq_restore(irq_state);

    ntos_port_request_context_switch();
}

void ntos_task_block_current(ntos_list_t *wait_list, ntos_block_reason_t reason)
{
    NTOS_ASSERT(wait_list != NULL);
    NTOS_ASSERT((reason != NTOS_BLOCK_NONE) && (reason != NTOS_BLOCK_DELAY));
    NTOS_ASSERT(ntos_current_tcb != NULL);
    NTOS_ASSERT(ntos_current_tcb->state == NTOS_TASK_RUNNING); /* chỉ gọi từ task context */
    NTOS_ASSERT(ntos_current_tcb != &ntos_idle_task_tcb);      /* idle không được block */
    NTOS_ASSERT(ntos_current_tcb->wait_item.container == NULL); /* một wait_item chỉ ở tối đa một wait list */

    /* Caller (ntos_sem_take()/ntos_queue_send()/ntos_queue_receive()) tự chịu
     * trách nhiệm giữ critical section quanh lời gọi này và tự
     * ntos_port_request_context_switch() sau khi rời critical section - hàm
     * này không tự request PendSV, giống quy ước ntos_delay(). */
    ntos_list_remove(&ntos_current_tcb->ready_item);
    ntos_current_tcb->state = NTOS_TASK_BLOCKED;
    ntos_current_tcb->block_reason = reason;
    ntos_list_push_back(wait_list, &ntos_current_tcb->wait_item);

    NTOS_ASSERT(ntos_current_tcb->ready_item.container == NULL);
    NTOS_ASSERT(ntos_current_tcb->wait_item.container == wait_list);
}

void ntos_task_make_ready(ntos_tcb_t *task)
{
    NTOS_ASSERT(task != NULL);
    NTOS_ASSERT(task->state == NTOS_TASK_BLOCKED);
    NTOS_ASSERT((task->block_reason != NTOS_BLOCK_NONE) && (task->block_reason != NTOS_BLOCK_DELAY));
    NTOS_ASSERT(task->ready_item.container == NULL);
    NTOS_ASSERT(task->wait_item.container != NULL);

    /* Dùng chung cho cả wake receiver/sender (queue) lẫn wake waiter
     * (semaphore) - chỉ đưa task READY, không copy/handoff dữ liệu gì (đó là
     * việc riêng của từng module gọi nó). */
    ntos_list_remove(&task->wait_item);
    task->block_reason = NTOS_BLOCK_NONE;
    task->state = NTOS_TASK_READY;
    ntos_list_push_back(&ready_lists[task->priority], &task->ready_item);

    NTOS_ASSERT(task->wait_item.container == NULL);
    NTOS_ASSERT(task->ready_item.container == &ready_lists[task->priority]);
}
