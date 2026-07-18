/**
 * @file ntos_assert.c
 * @brief Xử lý khi NTOS_ASSERT fail.
 */
#include "ntos_assert_internal.h"

void ntos_assert_failed(const char *file, int line, const char *expression)
{
    (void)file;
    (void)line;
    (void)expression;
    /* Tắt interrupt rồi treo vô hạn kèm breakpoint - dừng hẳn hệ thống thay vì
     * chạy tiếp với state đã sai, để debugger bắt được đúng điểm assert fail. */
    __asm volatile("cpsid i" ::: "memory");
    for (;;)
    {
        __asm volatile("bkpt 0");
    }
}
