#ifndef NTOS_ASSERT_INTERNAL_H
#define NTOS_ASSERT_INTERNAL_H

/**
 * @file ntos_assert_internal.h
 * @brief Macro assert dùng nội bộ kernel/port - dừng chương trình khi điều kiện sai.
 */

/** @brief Xử lý khi assert fail; định nghĩa tại kernel/ntos_assert.c (hiện: cấm interrupt rồi treo vô hạn). */
void ntos_assert_failed(const char *file, int line, const char *expression);

/** @brief Kiểm tra expr, gọi ntos_assert_failed() kèm vị trí source nếu expr sai. */
#define NTOS_ASSERT(expr)                         \
    do                                            \
    {                                             \
        if (!(expr))                              \
        {                                         \
            ntos_assert_failed(                   \
                __FILE__, __LINE__, #expr);       \
        }                                         \
    } while (0)

#endif
