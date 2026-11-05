/** @file queue.c
 *  @brief Static message queue for task and ISR contexts.
 */
#include "queue.h"
#include "kernel_internal.h"
#include "rtos_port.h"

#include <string.h>

rtos_status_t rtos_queue_init_static(rtos_queue_t *queue, void *storage, size_t item_size,
                                     size_t capacity)
{
    if ((queue == NULL) || (storage == NULL) || (item_size == 0U) || (capacity == 0U))
    {
        return RTOS_ERR_INVALID_ARGUMENT;
    }

    queue->storage = (uint8_t *)storage;
    queue->item_size = item_size;
    queue->capacity = capacity;
    queue->read_index = 0U;
    queue->write_index = 0U;
    queue->count = 0U;
    rtos_list_init(&queue->waiting_receivers);
    rtos_list_init(&queue->waiting_senders);

    return RTOS_OK;
}
rtos_status_t rtos_queue_send(rtos_queue_t *queue, const void *item, rtos_tick_t timeout)
{
    rtos_irq_state_t irq_state;

    if ((queue == NULL) || (item == NULL))
    {
        return RTOS_ERR_INVALID_ARGUMENT;
    }

    /* Wake không đặt chỗ (reserve slot) cho sender này - nó chỉ được chuyển
     * READY, chưa chắc chạy lại ngay. Một sender khác có thể được lập lịch
     * trước và giành mất chỗ trống vừa rảnh ra. Vì vậy phải quay lại đầu vòng
     * lặp để tự kiểm tra `count < capacity` lần nữa mỗi khi được wake, thay vì
     * coi việc được wake là bằng chứng đã có chỗ để ghi. */
    for (;;)
    {
        irq_state = rtos_port_irq_save();

        if (queue->count < queue->capacity)
        {
            (void)memcpy(&queue->storage[queue->write_index * queue->item_size], item,
                         queue->item_size);
            queue->write_index = (queue->write_index + 1U) % queue->capacity;
            queue->count++;

            if (!rtos_list_is_empty(&queue->waiting_receivers))
            {
                rtos_tcb_t *receiver = rtos_waiter_select_highest(&queue->waiting_receivers);
                bool need_switch;

                rtos_task_make_ready(receiver);

                /* rtos_queue_send() chỉ được gọi từ task context, nên luôn có
                 * một task hiện hành để so priority với receiver vừa wake. */
                RTOS_ASSERT(rtos_current_tcb != NULL);
                need_switch = (receiver->priority > rtos_current_tcb->priority);

                rtos_port_irq_restore(irq_state);

                if (need_switch)
                {
                    rtos_port_request_context_switch();
                }

                return RTOS_OK;
            }

            rtos_port_irq_restore(irq_state);
            return RTOS_OK;
        }

        if (timeout == RTOS_NO_WAIT)
        {
            rtos_port_irq_restore(irq_state);
            return RTOS_ERR_QUEUE_FULL;
        }

        /* Không có đường wake-theo-thời-hạn cho sender đang chờ (chỉ
         * rtos_tick_handler() wake task delay) - mọi giá trị timeout khác 0
         * đều được xử lý như chờ vô hạn. */
        rtos_task_block_current(&queue->waiting_senders, RTOS_BLOCK_QUEUE_SEND);

        rtos_port_irq_restore(irq_state);

        rtos_port_request_context_switch();
    }
}

rtos_status_t rtos_queue_receive(rtos_queue_t *queue, void *item, rtos_tick_t timeout)
{
    rtos_irq_state_t irq_state;

    if ((queue == NULL) || (item == NULL))
    {
        return RTOS_ERR_INVALID_ARGUMENT;
    }

    /* Wake không giao item (handoff) cho receiver này - nó chỉ được chuyển
     * READY, chưa chắc chạy lại ngay, và một receiver khác có thể chạy trước
     * rồi lấy mất item vừa được gửi vào. Vì vậy phải quay lại đầu vòng lặp
     * để tự kiểm tra `count > 0` lần nữa mỗi khi được wake, thay vì trả
     * RTOS_OK ngay sau khi resume mà không xác nhận item còn tồn tại. */
    for (;;)
    {
        irq_state = rtos_port_irq_save();

        if (queue->count > 0U)
        {
            (void)memcpy(item, &queue->storage[queue->read_index * queue->item_size],
                         queue->item_size);
            queue->read_index = (queue->read_index + 1U) % queue->capacity;
            queue->count--;

            if (!rtos_list_is_empty(&queue->waiting_senders))
            {
                rtos_tcb_t *sender = rtos_waiter_select_highest(&queue->waiting_senders);
                bool need_switch;

                rtos_task_make_ready(sender);

                /* rtos_queue_receive() chỉ được gọi từ task context, nên luôn
                 * có một task hiện hành để so priority với sender vừa wake. */
                RTOS_ASSERT(rtos_current_tcb != NULL);
                need_switch = (sender->priority > rtos_current_tcb->priority);

                rtos_port_irq_restore(irq_state);

                if (need_switch)
                {
                    rtos_port_request_context_switch();
                }

                return RTOS_OK;
            }

            rtos_port_irq_restore(irq_state);
            return RTOS_OK;
        }

        if (timeout == RTOS_NO_WAIT)
        {
            rtos_port_irq_restore(irq_state);
            return RTOS_ERR_QUEUE_EMPTY;
        }

        rtos_task_block_current(&queue->waiting_receivers, RTOS_BLOCK_QUEUE_RECEIVE);

        rtos_port_irq_restore(irq_state);

        rtos_port_request_context_switch();
    }
}

rtos_status_t rtos_queue_send_from_isr(rtos_queue_t *queue, const void *item,
                                       bool *higher_priority_task_woken)
{
    rtos_irq_state_t irq_state;
    rtos_status_t status;

    if ((queue == NULL) || (item == NULL) || (higher_priority_task_woken == NULL))
    {
        return RTOS_ERR_INVALID_ARGUMENT;
    }

    *higher_priority_task_woken = false;

    irq_state = rtos_port_irq_save();

    if (queue->count < queue->capacity)
    {
        (void)memcpy(&queue->storage[queue->write_index * queue->item_size], item,
                     queue->item_size);
        queue->write_index = (queue->write_index + 1U) % queue->capacity;
        queue->count++;

        if (!rtos_list_is_empty(&queue->waiting_receivers))
        {
            rtos_tcb_t *receiver = rtos_waiter_select_highest(&queue->waiting_receivers);

            rtos_task_make_ready(receiver);

            /* Nếu ISR chạy trước khi task đầu tiên từng được chọn,
             * rtos_current_tcb chưa trỏ tới task hợp lệ nào - so priority với
             * nó lúc đó sẽ đọc dữ liệu rác. Chặn bằng rtos_scheduler_running
             * thay vì giả định caller luôn bật ngắt sau khi scheduler chạy:
             * receiver vẫn được chuyển READY bình thường ở trên, chỉ riêng
             * phần "có cần preempt ai không" bị bỏ qua vì chưa có câu trả lời
             * hợp lệ. */
            if (rtos_scheduler_running && (receiver->priority > rtos_current_tcb->priority))
            {
                *higher_priority_task_woken = true;
            }
        }

        status = RTOS_OK;
    }
    else
    {
        status = RTOS_ERR_QUEUE_FULL;
    }

    rtos_port_irq_restore(irq_state);

    if (*higher_priority_task_woken)
    {
        rtos_port_request_context_switch();
    }

    return status;
}
