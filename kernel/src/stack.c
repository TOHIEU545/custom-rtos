/** @file stack.c
 *  @brief Stack fill, high-water mark and guard diagnostics.
 */
#include "stack.h"
#include "config.h"
#include "kernel_internal.h"

rtos_status_t rtos_task_stack_get_info(const rtos_tcb_t *tcb, rtos_task_stack_info_t *info)
{
    if ((tcb == NULL) || (info == NULL))
    {
        return RTOS_ERR_INVALID_ARGUMENT;
    }
    if (tcb->state == RTOS_TASK_UNUSED)
    {
        return RTOS_ERR_INVALID_STATE;
    }

#if RTOS_ENABLE_STACK_FILL
    size_t untouched;

    RTOS_ASSERT(tcb->stack_low != NULL);
    RTOS_ASSERT(tcb->stack_high > tcb->stack_low);
    RTOS_ASSERT((size_t)(tcb->stack_high - tcb->stack_low) == tcb->stack_words);
    /* stack_pointer nằm ngoài vùng stack nghĩa là TCB/stack đã bị hỏng - phát
     * hiện sớm ở đây thay vì để PendSV restore một context rác. */
    RTOS_ASSERT(tcb->stack_pointer >= tcb->stack_low);
    RTOS_ASSERT(tcb->stack_pointer < tcb->stack_high);

    /* Quét từ đáy (stack_low) lên - word nào vẫn giữ đúng pattern nghĩa là chưa
     * từng bị task ghi đè. Dừng ở word đầu tiên khác pattern: high-water mark là
     * phần còn lại phía trên, không phải "đếm toàn bộ word khác pattern" (một
     * task có thể ghi giá trị trùng ngẫu nhiên 0xA5A5A5A5 vào một chỗ rồi thôi -
     * không quan trọng, vì word đó chắc chắn nằm bên trong vùng đã từng được
     * dùng, tính từ ranh giới liền mạch đầu tiên là đủ và đơn giản). */
    untouched = 0U;
    while ((untouched < tcb->stack_words) && (tcb->stack_low[untouched] == RTOS_STACK_FILL_PATTERN))
    {
        untouched++;
    }

    info->stack_words = tcb->stack_words;
    info->untouched_words = untouched;
    info->high_water_mark_words = tcb->stack_words - untouched;

#if RTOS_ENABLE_STACK_GUARD
    /* Guard chỉ là word đáy cùng (index 0) - nếu nó đã bị ghi đè thì untouched == 0. */
    info->guard_intact = (untouched > 0U);
#else
    info->guard_intact =
        true; /* Guard checking tắt trong build này - không xác định, coi như OK. */
#endif

    return RTOS_OK;
#else
    (void)tcb;
    (void)info;
    return RTOS_ERR_INVALID_STATE;
#endif
}

bool rtos_task_stack_guard_ok(const rtos_tcb_t *tcb)
{
    RTOS_ASSERT(tcb != NULL);

    if (tcb->state == RTOS_TASK_UNUSED)
    {
        return false;
    }

#if RTOS_ENABLE_STACK_GUARD && RTOS_ENABLE_STACK_FILL
    RTOS_ASSERT(tcb->stack_low != NULL);
    return tcb->stack_low[0] == RTOS_STACK_FILL_PATTERN;
#else
    return false; /* Không thể xác định nếu guard/fill đang tắt. */
#endif
}
