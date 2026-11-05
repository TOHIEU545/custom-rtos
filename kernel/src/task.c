/**
 * @file task.c
 * @brief Tạo task tĩnh, yield, delay và chuyển trạng thái task.
 */
#include "task.h"
#include "stack.h"
#include "config.h"
#include "kernel_internal.h"
#include "rtos_port.h"

#include <stddef.h>
#include <stdint.h>

/* Giới hạn của rtos_delay(): phép so sánh tick wraparound-safe (rtos_tick_reached())
 * dùng signed subtraction nên chỉ đúng với độ lệch <= INT32_MAX tick. */
#define RTOS_DELAY_MAX_TICKS ((rtos_tick_t)INT32_MAX)

rtos_status_t rtos_task_create_static(rtos_tcb_t *tcb, const char *name, rtos_task_entry_t entry,
                                      void *argument, uint32_t *stack, size_t stack_words,
                                      rtos_priority_t priority)
{
    if ((tcb == NULL) || (stack == NULL) || (entry == NULL))
    {
        return RTOS_ERR_INVALID_ARGUMENT;
    }
    if (priority >= RTOS_MAX_PRIORITIES)
    {
        return RTOS_ERR_INVALID_PRIORITY;
    }
    if (stack_words < RTOS_PORT_INITIAL_FRAME_WORDS)
    {
        return RTOS_ERR_INVALID_ARGUMENT;
    }
    if (((uintptr_t)stack & 0x7U) != 0U)
    {
        return RTOS_ERR_INVALID_ARGUMENT;
    }
    if ((stack_words & 0x1U) != 0U)
    {
        return RTOS_ERR_INVALID_ARGUMENT;
    }
    if (tcb->state != RTOS_TASK_UNUSED)
    {
        return RTOS_ERR_INVALID_STATE;
    }

#if RTOS_ENABLE_STACK_FILL
    /* Fill pattern toàn bộ stack để sau này rtos_task_stack_get_info()
     * trong file này đo được high-water mark/guard. */
    for (size_t i = 0; i < stack_words; ++i)
    {
        stack[i] = RTOS_STACK_FILL_PATTERN;
    }
#endif

    tcb->stack_pointer = rtos_port_stack_init(stack, stack_words, entry, argument);
    tcb->stack_low = stack;
    tcb->stack_high = stack + stack_words;
    tcb->stack_words = stack_words;
    tcb->entry = entry;
    tcb->argument = argument;
    tcb->name = name;
    tcb->priority = priority;
    tcb->state = RTOS_TASK_READY;
    tcb->block_reason = RTOS_BLOCK_NONE; /* Chỉ có ý nghĩa khi state == BLOCKED. */
    tcb->wake_tick = 0U; /* Chỉ có ý nghĩa khi block_reason == RTOS_BLOCK_DELAY. */
    rtos_list_item_init(&tcb->ready_item, tcb);
    rtos_list_item_init(&tcb->wait_item, tcb);

    /* rtos_scheduler_register_task() (kernel/src/core.c) vừa đăng ký vào
     * task_table[] (còn dùng cho tick handler quét wake) vừa chèn ready_item
     * vào ready_lists[priority]. */
    rtos_scheduler_register_task(tcb);

    return RTOS_OK;
}
void rtos_yield(void)
{
    rtos_port_request_context_switch();
}

void rtos_delay(rtos_tick_t ticks)
{
    rtos_irq_state_t irq_state;

    if (ticks == 0U)
    {
        /* Quy ước: delay(0) tương đương yield, không block. */
        rtos_yield();
        return;
    }

    RTOS_ASSERT(ticks <= RTOS_DELAY_MAX_TICKS);
    RTOS_ASSERT(rtos_current_tcb != NULL);
    RTOS_ASSERT(rtos_current_tcb->state == RTOS_TASK_RUNNING); /* chỉ gọi từ task context */
    RTOS_ASSERT(rtos_current_tcb != &rtos_idle_task_tcb);      /* idle không được block */
    /* Task delay không được đồng thời chờ IPC nào - hai cơ chế block hoàn
     * toàn tách biệt (xem block_reason trong kernel/include/rtos.h). */
    RTOS_ASSERT(rtos_current_tcb->wait_item.container == NULL);

    /* Critical section ngắn: chỉ bảo vệ wake_tick + state + ready_item của
     * chính task hiện tại khỏi bị SysTick xen vào giữa chừng (nếu PendSV chạy
     * trước khi state kịp chuyển BLOCKED, scheduler sẽ demote nhầm về READY
     * thay vì giữ BLOCKED - mất delay). Không giữ IRQ tắt qua lúc request
     * context switch. */
    irq_state = rtos_port_irq_save();

    rtos_current_tcb->wake_tick = rtos_tick_get() + ticks;
    rtos_current_tcb->state = RTOS_TASK_BLOCKED;
    rtos_current_tcb->block_reason = RTOS_BLOCK_DELAY;
    /* BLOCKED task không được nằm trong ready list - rtos_tick_handler()
     * sẽ rtos_list_push_back() lại khi wake đúng hạn. */
    rtos_list_remove(&rtos_current_tcb->ready_item);

    /* Assert BLOCKED task tuyệt đối không được còn container - nếu còn nghĩa
     * là rtos_list_remove() thất bại hoặc bị gọi sai thứ tự. */
    RTOS_ASSERT(rtos_current_tcb->ready_item.container == NULL);

    rtos_port_irq_restore(irq_state);

    rtos_port_request_context_switch();
}

void rtos_task_block_current(rtos_list_t *wait_list, rtos_block_reason_t reason)
{
    RTOS_ASSERT(wait_list != NULL);
    RTOS_ASSERT((reason != RTOS_BLOCK_NONE) && (reason != RTOS_BLOCK_DELAY));
    RTOS_ASSERT(rtos_current_tcb != NULL);
    RTOS_ASSERT(rtos_current_tcb->state == RTOS_TASK_RUNNING); /* chỉ gọi từ task context */
    RTOS_ASSERT(rtos_current_tcb != &rtos_idle_task_tcb);      /* idle không được block */
    RTOS_ASSERT(rtos_current_tcb->wait_item.container ==
                NULL); /* một wait_item chỉ ở tối đa một wait list */

    /* Caller (rtos_sem_take()/rtos_queue_send()/rtos_queue_receive()) tự chịu
     * trách nhiệm giữ critical section quanh lời gọi này và tự
     * rtos_port_request_context_switch() sau khi rời critical section - hàm
     * này không tự request PendSV, giống quy ước rtos_delay(). */
    rtos_list_remove(&rtos_current_tcb->ready_item);
    rtos_current_tcb->state = RTOS_TASK_BLOCKED;
    rtos_current_tcb->block_reason = reason;
    rtos_list_push_back(wait_list, &rtos_current_tcb->wait_item);

    RTOS_ASSERT(rtos_current_tcb->ready_item.container == NULL);
    RTOS_ASSERT(rtos_current_tcb->wait_item.container == wait_list);
}

void rtos_task_make_ready(rtos_tcb_t *task)
{
    RTOS_ASSERT(task != NULL);
    RTOS_ASSERT(task->state == RTOS_TASK_BLOCKED);
    RTOS_ASSERT((task->block_reason != RTOS_BLOCK_NONE) &&
                (task->block_reason != RTOS_BLOCK_DELAY));
    RTOS_ASSERT(task->ready_item.container == NULL);
    RTOS_ASSERT(task->wait_item.container != NULL);

    /* Chỉ chuyển state/list membership, không copy hay giao (handoff) bất kỳ
     * dữ liệu nào - bán kính ảnh hưởng của hàm này dừng lại ở "task chạy được
     * chưa", còn "task chạy lại có thực sự lấy được thứ nó cần không" là việc
     * riêng của caller (token semaphore, item trong buffer). Nhờ giữ ranh
     * giới rõ như vậy, cùng một hàm mới dùng chung được cho semaphore
     * (handoff trực tiếp) lẫn queue (phải tự re-check sau khi chạy lại) dù
     * hai bên có ngữ nghĩa khác nhau. */
    rtos_list_remove(&task->wait_item);
    task->block_reason = RTOS_BLOCK_NONE;
    task->state = RTOS_TASK_READY;
    rtos_list_push_back(&ready_lists[task->priority], &task->ready_item);

    RTOS_ASSERT(task->wait_item.container == NULL);
    RTOS_ASSERT(task->ready_item.container == &ready_lists[task->priority]);
}

rtos_tcb_t *rtos_waiter_select_highest(rtos_list_t *waiting_tasks)
{
    rtos_list_item_t *item;
    rtos_tcb_t *highest = NULL;

    RTOS_ASSERT(!rtos_list_is_empty(waiting_tasks));

    for (item = waiting_tasks->end.next; item != &waiting_tasks->end; item = item->next)
    {
        rtos_tcb_t *candidate = (rtos_tcb_t *)item->owner;

        RTOS_ASSERT(candidate != NULL);
        if ((highest == NULL) || (candidate->priority > highest->priority))
        {
            highest = candidate;
        }
    }

    RTOS_ASSERT(highest != NULL);
    return highest;
}
