/**
 * @file ntos_sem.c
 * @brief Semaphore đếm: take/give với direct handoff, give từ ISR không block.
 */
#include <ntos/ntos_sem.h>

#include "ntos_assert_internal.h"
#include "ntos_kernel_internal.h"
#include "ntos_list_internal.h"
#include "ntos_port.h"

#include <stddef.h>

/**
 * @brief Quét waiting_tasks tìm task priority cao nhất; cùng priority thì giữ
 *        task xuất hiện trước trong list (FIFO) vì chỉ dùng so sánh "lớn hơn
 *        nghiêm ngặt" - task đầu tiên đạt priority cao nhất không bị thay thế
 *        bởi task sau cùng priority.
 */
static ntos_tcb_t *sem_select_highest_waiter(ntos_semaphore_t *sem)
{
    ntos_list_item_t *item;
    ntos_tcb_t *highest = NULL;

    NTOS_ASSERT(!ntos_list_is_empty(&sem->waiting_tasks));

    for (item = sem->waiting_tasks.end.next; item != &sem->waiting_tasks.end; item = item->next)
    {
        ntos_tcb_t *candidate = (ntos_tcb_t *)item->owner;

        NTOS_ASSERT(candidate != NULL);

        if ((highest == NULL) || (candidate->priority > highest->priority))
        {
            highest = candidate;
        }
    }

    NTOS_ASSERT(highest != NULL);
    return highest;
}

void ntos_sem_init(ntos_semaphore_t *sem, uint32_t initial_count, uint32_t max_count)
{
    NTOS_ASSERT(sem != NULL);
    NTOS_ASSERT(max_count > 0U);
    NTOS_ASSERT(initial_count <= max_count);

    sem->count = initial_count;
    sem->max_count = max_count;
    ntos_list_init(&sem->waiting_tasks);
}

ntos_status_t ntos_sem_take(ntos_semaphore_t *sem, ntos_tick_t timeout)
{
    ntos_irq_state_t irq_state;

    if (sem == NULL)
    {
        return NTOS_ERR_INVALID_ARGUMENT;
    }

    irq_state = ntos_port_irq_save();

    if (sem->count > 0U)
    {
        sem->count--;
        ntos_port_irq_restore(irq_state);
        return NTOS_OK;
    }

    if (timeout == NTOS_NO_WAIT)
    {
        ntos_port_irq_restore(irq_state);
        return NTOS_ERR_TIMEOUT;
    }

    /* Không có đường wake-theo-thời-hạn cho task đang chờ semaphore (chỉ
     * ntos_tick_handler() wake task delay) - nên mọi giá trị timeout khác 0
     * đều được xử lý như chờ vô hạn thay vì âm thầm coi là timeout hữu hạn
     * không hỗ trợ. */
    ntos_task_block_current(&sem->waiting_tasks, NTOS_BLOCK_SEMAPHORE);

    ntos_port_irq_restore(irq_state);

    ntos_port_request_context_switch();

    /* Task chỉ có thể chạy lại tới điểm này sau khi một give() khác chọn
     * chính nó làm waiter và chuyển nó READY - tức token đã được giao thẳng
     * cho task này (direct handoff). Không trừ count ở đây: count chỉ tăng
     * khi give() không có waiter nào, nên không có gì để trừ - trừ thêm sẽ
     * làm count âm hoặc trừ nhầm phần dành cho lượt take() khác. */
    return NTOS_OK;
}

ntos_status_t ntos_sem_give(ntos_semaphore_t *sem)
{
    ntos_irq_state_t irq_state;
    ntos_tcb_t *waiter;
    bool need_switch = false;
    ntos_priority_t waiter_priority = 0U;

    if (sem == NULL)
    {
        return NTOS_ERR_INVALID_ARGUMENT;
    }

    irq_state = ntos_port_irq_save();

    if (!ntos_list_is_empty(&sem->waiting_tasks))
    {
        /* Token đi thẳng cho waiter thay vì tăng count rồi để waiter tự lấy
         * lại - nếu làm cả hai, cùng một give() sẽ bị tính hai lần (một lần
         * nằm trong count, một lần đã trao cho waiter), cho phép một take()
         * khác lấy nhầm phần chưa thuộc về nó. */
        waiter = sem_select_highest_waiter(sem);
        waiter_priority = waiter->priority;
        ntos_task_make_ready(waiter);

        /* ntos_sem_give() chỉ được gọi từ task context, nên luôn có một task
         * hiện hành để so priority - khác ntos_sem_give_from_isr(), nơi ISR
         * có thể chạy trước khi task đầu tiên từng được chọn. */
        NTOS_ASSERT(ntos_current_tcb != NULL);
        if (waiter_priority > ntos_current_tcb->priority)
        {
            need_switch = true;
        }

        ntos_port_irq_restore(irq_state);

        if (need_switch)
        {
            ntos_port_request_context_switch();
        }

        return NTOS_OK;
    }

    if (sem->count < sem->max_count)
    {
        sem->count++;
        ntos_port_irq_restore(irq_state);
        return NTOS_OK;
    }

    ntos_port_irq_restore(irq_state);
    return NTOS_ERR_RESOURCE_FULL;
}

ntos_status_t ntos_sem_give_from_isr(ntos_semaphore_t *sem, bool *higher_priority_task_woken)
{
    ntos_irq_state_t irq_state;
    ntos_status_t status;
    bool need_switch = false;

    if ((sem == NULL) || (higher_priority_task_woken == NULL))
    {
        return NTOS_ERR_INVALID_ARGUMENT;
    }

    *higher_priority_task_woken = false;

    irq_state = ntos_port_irq_save();

    if (!ntos_list_is_empty(&sem->waiting_tasks))
    {
        ntos_tcb_t *waiter = sem_select_highest_waiter(sem);

        ntos_task_make_ready(waiter);

        /* Nếu ISR chạy trước khi task đầu tiên từng được chọn, ntos_current_tcb
         * chưa trỏ tới một task hợp lệ nào - so priority với nó lúc đó sẽ đọc
         * dữ liệu rác. Chặn bằng ntos_scheduler_running thay vì dereference rồi
         * assert non-NULL: waiter vẫn được chuyển READY bình thường ở trên,
         * chỉ riêng phần "có cần preempt ai không" là chưa có câu trả lời hợp lệ
         * nên bỏ qua, không phải lỗi. */
        if (ntos_scheduler_running && (waiter->priority > ntos_current_tcb->priority))
        {
            *higher_priority_task_woken = true;
            need_switch = true;
        }

        status = NTOS_OK;
    }
    else if (sem->count < sem->max_count)
    {
        sem->count++;
        status = NTOS_OK;
    }
    else
    {
        status = NTOS_ERR_RESOURCE_FULL;
    }

    ntos_port_irq_restore(irq_state);

    /* Không tự context switch trong ISR - chỉ request PendSV, switch thật xảy
     * ra sau khi ISR return (tail-chaining), giữ đúng "PendSV là nơi duy nhất
     * context switch". */
    if (need_switch)
    {
        ntos_port_request_context_switch();
    }

    return status;
}
