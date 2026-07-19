/**
 * @file ntos_queue.c
 * @brief Queue ring buffer tĩnh: send/receive blocking đối xứng, send từ ISR không block.
 */
#include <ntos/ntos_queue.h>

#include "ntos_assert_internal.h"
#include "ntos_kernel_internal.h"
#include "ntos_list_internal.h"
#include "ntos_port.h"

#include <stddef.h>
#include <string.h>

/**
 * @brief Quét một wait list (waiting_receivers hoặc waiting_senders) tìm task
 *        priority cao nhất; cùng priority thì giữ task xuất hiện trước trong
 *        list (FIFO) - giống hệt logic sem_select_highest_waiter()
 *        (kernel/ntos_sem.c).
 */
static ntos_tcb_t *queue_select_highest_waiter(ntos_list_t *waiting_tasks)
{
    ntos_list_item_t *item;
    ntos_tcb_t *highest = NULL;

    NTOS_ASSERT(!ntos_list_is_empty(waiting_tasks));

    for (item = waiting_tasks->end.next; item != &waiting_tasks->end; item = item->next)
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

ntos_status_t ntos_queue_init_static(ntos_queue_t *queue, void *storage, size_t item_size, size_t capacity)
{
    if ((queue == NULL) || (storage == NULL) || (item_size == 0U) || (capacity == 0U))
    {
        return NTOS_ERR_INVALID_ARGUMENT;
    }

    queue->storage = (uint8_t *)storage;
    queue->item_size = item_size;
    queue->capacity = capacity;
    queue->read_index = 0U;
    queue->write_index = 0U;
    queue->count = 0U;
    ntos_list_init(&queue->waiting_receivers);
    ntos_list_init(&queue->waiting_senders);

    return NTOS_OK;
}

ntos_status_t ntos_queue_send(ntos_queue_t *queue, const void *item, ntos_tick_t timeout)
{
    ntos_irq_state_t irq_state;

    if ((queue == NULL) || (item == NULL))
    {
        return NTOS_ERR_INVALID_ARGUMENT;
    }

    /* Wake không đặt chỗ (reserve slot) cho sender này - nó chỉ được chuyển
     * READY, chưa chắc chạy lại ngay. Một sender khác có thể được lập lịch
     * trước và giành mất chỗ trống vừa rảnh ra. Vì vậy phải quay lại đầu vòng
     * lặp để tự kiểm tra `count < capacity` lần nữa mỗi khi được wake, thay vì
     * coi việc được wake là bằng chứng đã có chỗ để ghi. */
    for (;;)
    {
        irq_state = ntos_port_irq_save();

        if (queue->count < queue->capacity)
        {
            (void)memcpy(&queue->storage[queue->write_index * queue->item_size], item, queue->item_size);
            queue->write_index = (queue->write_index + 1U) % queue->capacity;
            queue->count++;

            if (!ntos_list_is_empty(&queue->waiting_receivers))
            {
                ntos_tcb_t *receiver = queue_select_highest_waiter(&queue->waiting_receivers);
                bool need_switch;

                ntos_task_make_ready(receiver);

                /* ntos_queue_send() chỉ được gọi từ task context, nên luôn có
                 * một task hiện hành để so priority với receiver vừa wake. */
                NTOS_ASSERT(ntos_current_tcb != NULL);
                need_switch = (receiver->priority > ntos_current_tcb->priority);

                ntos_port_irq_restore(irq_state);

                if (need_switch)
                {
                    ntos_port_request_context_switch();
                }

                return NTOS_OK;
            }

            ntos_port_irq_restore(irq_state);
            return NTOS_OK;
        }

        if (timeout == NTOS_NO_WAIT)
        {
            ntos_port_irq_restore(irq_state);
            return NTOS_ERR_QUEUE_FULL;
        }

        /* Không có đường wake-theo-thời-hạn cho sender đang chờ (chỉ
         * ntos_tick_handler() wake task delay) - mọi giá trị timeout khác 0
         * đều được xử lý như chờ vô hạn. */
        ntos_task_block_current(&queue->waiting_senders, NTOS_BLOCK_QUEUE_SEND);

        ntos_port_irq_restore(irq_state);

        ntos_port_request_context_switch();
    }
}

ntos_status_t ntos_queue_receive(ntos_queue_t *queue, void *item, ntos_tick_t timeout)
{
    ntos_irq_state_t irq_state;

    if ((queue == NULL) || (item == NULL))
    {
        return NTOS_ERR_INVALID_ARGUMENT;
    }

    /* Wake không giao item (handoff) cho receiver này - nó chỉ được chuyển
     * READY, chưa chắc chạy lại ngay, và một receiver khác có thể chạy trước
     * rồi lấy mất item vừa được gửi vào. Vì vậy phải quay lại đầu vòng lặp
     * để tự kiểm tra `count > 0` lần nữa mỗi khi được wake, thay vì trả
     * NTOS_OK ngay sau khi resume mà không xác nhận item còn tồn tại. */
    for (;;)
    {
        irq_state = ntos_port_irq_save();

        if (queue->count > 0U)
        {
            (void)memcpy(item, &queue->storage[queue->read_index * queue->item_size], queue->item_size);
            queue->read_index = (queue->read_index + 1U) % queue->capacity;
            queue->count--;

            if (!ntos_list_is_empty(&queue->waiting_senders))
            {
                ntos_tcb_t *sender = queue_select_highest_waiter(&queue->waiting_senders);
                bool need_switch;

                ntos_task_make_ready(sender);

                /* ntos_queue_receive() chỉ được gọi từ task context, nên luôn
                 * có một task hiện hành để so priority với sender vừa wake. */
                NTOS_ASSERT(ntos_current_tcb != NULL);
                need_switch = (sender->priority > ntos_current_tcb->priority);

                ntos_port_irq_restore(irq_state);

                if (need_switch)
                {
                    ntos_port_request_context_switch();
                }

                return NTOS_OK;
            }

            ntos_port_irq_restore(irq_state);
            return NTOS_OK;
        }

        if (timeout == NTOS_NO_WAIT)
        {
            ntos_port_irq_restore(irq_state);
            return NTOS_ERR_QUEUE_EMPTY;
        }

        ntos_task_block_current(&queue->waiting_receivers, NTOS_BLOCK_QUEUE_RECEIVE);

        ntos_port_irq_restore(irq_state);

        ntos_port_request_context_switch();
    }
}

ntos_status_t ntos_queue_send_from_isr(ntos_queue_t *queue, const void *item, bool *higher_priority_task_woken)
{
    ntos_irq_state_t irq_state;
    ntos_status_t status;

    if ((queue == NULL) || (item == NULL) || (higher_priority_task_woken == NULL))
    {
        return NTOS_ERR_INVALID_ARGUMENT;
    }

    *higher_priority_task_woken = false;

    irq_state = ntos_port_irq_save();

    if (queue->count < queue->capacity)
    {
        (void)memcpy(&queue->storage[queue->write_index * queue->item_size], item, queue->item_size);
        queue->write_index = (queue->write_index + 1U) % queue->capacity;
        queue->count++;

        if (!ntos_list_is_empty(&queue->waiting_receivers))
        {
            ntos_tcb_t *receiver = queue_select_highest_waiter(&queue->waiting_receivers);

            ntos_task_make_ready(receiver);

            /* Nếu ISR chạy trước khi task đầu tiên từng được chọn,
             * ntos_current_tcb chưa trỏ tới task hợp lệ nào - so priority với
             * nó lúc đó sẽ đọc dữ liệu rác. Chặn bằng ntos_scheduler_running
             * thay vì giả định caller luôn bật ngắt sau khi scheduler chạy:
             * receiver vẫn được chuyển READY bình thường ở trên, chỉ riêng
             * phần "có cần preempt ai không" bị bỏ qua vì chưa có câu trả lời
             * hợp lệ. */
            if (ntos_scheduler_running && (receiver->priority > ntos_current_tcb->priority))
            {
                *higher_priority_task_woken = true;
            }
        }

        status = NTOS_OK;
    }
    else
    {
        status = NTOS_ERR_QUEUE_FULL;
    }

    ntos_port_irq_restore(irq_state);

    if (*higher_priority_task_woken)
    {
        ntos_port_request_context_switch();
    }

    return status;
}
