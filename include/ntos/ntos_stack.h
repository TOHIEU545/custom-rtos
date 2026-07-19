#ifndef NTOS_STACK_H
#define NTOS_STACK_H
/**
 * @file ntos_stack.h
 * @brief API công khai chẩn đoán stack: fill pattern, stack guard, high-water mark.
 */

#include <ntos/ntos_status.h>
#include <ntos/ntos_task.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/**
 * @brief Giá trị fill vào toàn bộ vùng stack lúc tạo task (khi NTOS_ENABLE_STACK_FILL).
 * @note Word nào vẫn giữ đúng giá trị này nghĩa là chưa từng bị task ghi đè -
 *       đây là cơ sở duy nhất để đo high-water mark, không dựa vào cơ chế nào khác.
 */
#define NTOS_STACK_FILL_PATTERN ((uint32_t)0xA5A5A5A5U)

/**
 * @brief Kết quả chẩn đoán stack của một task, đo tại thời điểm gọi.
 */
typedef struct ntos_task_stack_info
{
    size_t stack_words;           /**< Tổng số word của stack (== tcb->stack_words). */
    size_t high_water_mark_words; /**< Mức dùng CAO NHẤT từng quan sát được - không phải mức đang dùng ngay lúc gọi. */
    size_t untouched_words;       /**< Số word ở đáy stack (gần stack_low) chưa từng bị ghi đè, vẫn giữ NTOS_STACK_FILL_PATTERN. */
    bool guard_intact;            /**< false nếu word đáy stack (stack_low[0]) đã bị ghi đè - dấu hiệu overflow. */
} ntos_task_stack_info_t;

/**
 * @brief Quét vùng stack còn giữ NTOS_STACK_FILL_PATTERN để đo high-water mark và guard.
 *
 * @param tcb Task đã được tạo qua ntos_task_create_static() (state khác NTOS_TASK_UNUSED).
 * @param info Nơi ghi kết quả.
 * @return NTOS_OK nếu đo được; NTOS_ERR_INVALID_ARGUMENT nếu tcb/info NULL;
 *         NTOS_ERR_INVALID_STATE nếu tcb chưa được tạo, hoặc NTOS_ENABLE_STACK_FILL == 0
 *         (không có pattern nào để quét, kết quả sẽ vô nghĩa nên từ chối thẳng).
 *
 * @note Bán kính đo là high_water_mark_words = mức dùng CAO NHẤT từng quan sát
 *       được (peak), KHÔNG phải dung lượng đang dùng tại thời điểm gọi - task có
 *       thể đang dùng ít hơn ngay lúc này vì các hàm đã return.
 * @note O(stack_words) - KHÔNG gọi hàm này trong hot path (tick handler, PendSV);
 *       chỉ dùng cho debug/test/diagnostics, gọi khi cần.
 */
ntos_status_t ntos_task_stack_get_info(const ntos_tcb_t *tcb, ntos_task_stack_info_t *info);

/**
 * @brief Kiểm tra nhanh O(1) guard word ở đáy stack (stack_low[0]) - rẻ hơn
 *        ntos_task_stack_get_info() vì không quét toàn bộ stack.
 * @param tcb Task đã được tạo qua ntos_task_create_static().
 * @return true nếu guard word vẫn còn nguyên NTOS_STACK_FILL_PATTERN; false nếu đã
 *         bị ghi đè (dấu hiệu stack overflow), tcb chưa được tạo, hoặc
 *         NTOS_ENABLE_STACK_GUARD/NTOS_ENABLE_STACK_FILL == 0 (không kiểm tra được).
 * @note tcb == NULL là lỗi gọi hàm (assert), không phải giá trị hợp lệ để trả false.
 */
bool ntos_task_stack_guard_ok(const ntos_tcb_t *tcb);

#endif
