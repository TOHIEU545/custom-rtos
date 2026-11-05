/**
 * @file semaphore.c
 * @brief Counting semaphore for task and ISR contexts.
 */
#include "semaphore.h"
#include "kernel_internal.h"
#include "rtos_port.h"

void rtos_sem_init(rtos_semaphore_t *sem, uint32_t initial_count, uint32_t max_count)
{
    RTOS_ASSERT(sem != NULL);
    RTOS_ASSERT(max_count > 0U);
    RTOS_ASSERT(initial_count <= max_count);

    sem->count = initial_count;
    sem->max_count = max_count;
    rtos_list_init(&sem->waiting_tasks);
}
rtos_status_t rtos_sem_take(rtos_semaphore_t *sem, rtos_tick_t timeout)
{
    rtos_irq_state_t irq_state;

    if (sem == NULL)
    {
        return RTOS_ERR_INVALID_ARGUMENT;
    }

    irq_state = rtos_port_irq_save();

    if (sem->count > 0U)
    {
        sem->count--;
        rtos_port_irq_restore(irq_state);
        return RTOS_OK;
    }

    if (timeout == RTOS_NO_WAIT)
    {
        rtos_port_irq_restore(irq_state);
        return RTOS_ERR_TIMEOUT;
    }

    /* Không có đường wake-theo-thời-hạn cho task đang chờ semaphore (chỉ
     * rtos_tick_handler() wake task delay) - nên mọi giá trị timeout khác 0
     * đều được xử lý như chờ vô hạn thay vì âm thầm coi là timeout hữu hạn
     * không hỗ trợ. */
    rtos_task_block_current(&sem->waiting_tasks, RTOS_BLOCK_SEMAPHORE);

    rtos_port_irq_restore(irq_state);

    rtos_port_request_context_switch();

    /* Task chỉ có thể chạy lại tới điểm này sau khi một give() khác chọn
     * chính nó làm waiter và chuyển nó READY - tức token đã được giao thẳng
     * cho task này (direct handoff). Không trừ count ở đây: count chỉ tăng
     * khi give() không có waiter nào, nên không có gì để trừ - trừ thêm sẽ
     * làm count âm hoặc trừ nhầm phần dành cho lượt take() khác. */
    return RTOS_OK;
}

rtos_status_t rtos_sem_give(rtos_semaphore_t *sem)
{
    rtos_irq_state_t irq_state;
    rtos_tcb_t *waiter;
    bool need_switch = false;
    rtos_priority_t waiter_priority = 0U;

    if (sem == NULL)
    {
        return RTOS_ERR_INVALID_ARGUMENT;
    }

    irq_state = rtos_port_irq_save();

    if (!rtos_list_is_empty(&sem->waiting_tasks))
    {
        /* Token đi thẳng cho waiter thay vì tăng count rồi để waiter tự lấy
         * lại - nếu làm cả hai, cùng một give() sẽ bị tính hai lần (một lần
         * nằm trong count, một lần đã trao cho waiter), cho phép một take()
         * khác lấy nhầm phần chưa thuộc về nó. */
        waiter = rtos_waiter_select_highest(&sem->waiting_tasks);
        waiter_priority = waiter->priority;
        rtos_task_make_ready(waiter);

        /* rtos_sem_give() chỉ được gọi từ task context, nên luôn có một task
         * hiện hành để so priority - khác rtos_sem_give_from_isr(), nơi ISR
         * có thể chạy trước khi task đầu tiên từng được chọn. */
        RTOS_ASSERT(rtos_current_tcb != NULL);
        if (waiter_priority > rtos_current_tcb->priority)
        {
            need_switch = true;
        }

        rtos_port_irq_restore(irq_state);

        if (need_switch)
        {
            rtos_port_request_context_switch();
        }

        return RTOS_OK;
    }

    if (sem->count < sem->max_count)
    {
        sem->count++;
        rtos_port_irq_restore(irq_state);
        return RTOS_OK;
    }

    rtos_port_irq_restore(irq_state);
    return RTOS_ERR_RESOURCE_FULL;
}

rtos_status_t rtos_sem_give_from_isr(rtos_semaphore_t *sem, bool *higher_priority_task_woken)
{
    rtos_irq_state_t irq_state;
    rtos_status_t status;
    bool need_switch = false;

    if ((sem == NULL) || (higher_priority_task_woken == NULL))
    {
        return RTOS_ERR_INVALID_ARGUMENT;
    }

    *higher_priority_task_woken = false;

    irq_state = rtos_port_irq_save();

    if (!rtos_list_is_empty(&sem->waiting_tasks))
    {
        rtos_tcb_t *waiter = rtos_waiter_select_highest(&sem->waiting_tasks);

        rtos_task_make_ready(waiter);

        /* Nếu ISR chạy trước khi task đầu tiên từng được chọn, rtos_current_tcb
         * chưa trỏ tới một task hợp lệ nào - so priority với nó lúc đó sẽ đọc
         * dữ liệu rác. Chặn bằng rtos_scheduler_running thay vì dereference rồi
         * assert non-NULL: waiter vẫn được chuyển READY bình thường ở trên,
         * chỉ riêng phần "có cần preempt ai không" là chưa có câu trả lời hợp lệ
         * nên bỏ qua, không phải lỗi. */
        if (rtos_scheduler_running && (waiter->priority > rtos_current_tcb->priority))
        {
            *higher_priority_task_woken = true;
            need_switch = true;
        }

        status = RTOS_OK;
    }
    else if (sem->count < sem->max_count)
    {
        sem->count++;
        status = RTOS_OK;
    }
    else
    {
        status = RTOS_ERR_RESOURCE_FULL;
    }

    rtos_port_irq_restore(irq_state);

    /* Không tự context switch trong ISR - chỉ request PendSV, switch thật xảy
     * ra sau khi ISR return (tail-chaining), giữ đúng "PendSV là nơi duy nhất
     * context switch". */
    if (need_switch)
    {
        rtos_port_request_context_switch();
    }

    return status;
}
