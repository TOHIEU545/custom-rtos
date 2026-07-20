/**
 * @file ntos_stack.c
 * @brief Chẩn đoán stack: quét pattern NTOS_STACK_FILL_PATTERN để đo high-water
 *        mark và kiểm tra guard word ở đáy stack.
 */
#include <ntos/ntos_stack.h>

#include <ntos/ntos_config_default.h>

#include "ntos_assert_internal.h"

#include <stddef.h>

ntos_status_t ntos_task_stack_get_info(const ntos_tcb_t *tcb, ntos_task_stack_info_t *info)
{
    if ((tcb == NULL) || (info == NULL))
    {
        return NTOS_ERR_INVALID_ARGUMENT;
    }
    if (tcb->state == NTOS_TASK_UNUSED)
    {
        return NTOS_ERR_INVALID_STATE;
    }

#if NTOS_ENABLE_STACK_FILL
    size_t untouched;

    NTOS_ASSERT(tcb->stack_low != NULL);
    NTOS_ASSERT(tcb->stack_high > tcb->stack_low);
    NTOS_ASSERT((size_t)(tcb->stack_high - tcb->stack_low) == tcb->stack_words);
    /* stack_pointer nằm ngoài vùng stack nghĩa là TCB/stack đã bị hỏng - phát
     * hiện sớm ở đây thay vì để PendSV restore một context rác. */
    NTOS_ASSERT(tcb->stack_pointer >= tcb->stack_low);
    NTOS_ASSERT(tcb->stack_pointer < tcb->stack_high);

    /* Quét từ đáy (stack_low) lên - word nào vẫn giữ đúng pattern nghĩa là chưa
     * từng bị task ghi đè. Dừng ở word đầu tiên khác pattern: high-water mark là
     * phần còn lại phía trên, không phải "đếm toàn bộ word khác pattern" (một
     * task có thể ghi giá trị trùng ngẫu nhiên 0xA5A5A5A5 vào một chỗ rồi thôi -
     * không quan trọng, vì word đó chắc chắn nằm bên trong vùng đã từng được
     * dùng, tính từ ranh giới liền mạch đầu tiên là đủ và đơn giản). */
    untouched = 0U;
    while ((untouched < tcb->stack_words) && (tcb->stack_low[untouched] == NTOS_STACK_FILL_PATTERN))
    {
        untouched++;
    }

    info->stack_words = tcb->stack_words;
    info->untouched_words = untouched;
    info->high_water_mark_words = tcb->stack_words - untouched;

#if NTOS_ENABLE_STACK_GUARD
    /* Guard chỉ là word đáy cùng (index 0) - nếu nó đã bị ghi đè thì untouched == 0. */
    info->guard_intact = (untouched > 0U);
#else
    info->guard_intact =
        true; /* Guard checking tắt trong build này - không xác định, coi như OK. */
#endif

    return NTOS_OK;
#else
    (void)tcb;
    (void)info;
    return NTOS_ERR_INVALID_STATE;
#endif
}

bool ntos_task_stack_guard_ok(const ntos_tcb_t *tcb)
{
    NTOS_ASSERT(tcb != NULL);

    if (tcb->state == NTOS_TASK_UNUSED)
    {
        return false;
    }

#if NTOS_ENABLE_STACK_GUARD && NTOS_ENABLE_STACK_FILL
    NTOS_ASSERT(tcb->stack_low != NULL);
    return tcb->stack_low[0] == NTOS_STACK_FILL_PATTERN;
#else
    return false; /* Không thể xác định nếu guard/fill đang tắt. */
#endif
}
