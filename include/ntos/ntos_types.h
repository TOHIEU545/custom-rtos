#ifndef NTOS_TYPES_H
#define NTOS_TYPES_H

/**
 * @file ntos_types.h
 * @brief Định nghĩa các kiểu dữ liệu nền tảng dùng chung trong NTOS.
 */

#include <stdint.h>

/**
 * @brief Kiểu dữ liệu biểu diễn số tick của hệ thống.
 */
typedef uint32_t ntos_tick_t;

/**
 * @brief Kiểu dữ liệu biểu diễn độ ưu tiên của task.
 */
typedef uint8_t ntos_priority_t;

/**
 * @brief Kiểu hàm entry của một task.
 *
 * @param argument Con trỏ tới tham số được truyền cho task khi khởi tạo.
 */
typedef void (*ntos_task_entry_t)(void *argument);

/**
 * @brief Không chờ khi gọi một API có hỗ trợ timeout.
 */
#define NTOS_NO_WAIT ((ntos_tick_t)0U)

/**
 * @brief Chờ vô hạn khi gọi một API có hỗ trợ timeout.
 */
#define NTOS_WAIT_FOREVER ((ntos_tick_t)UINT32_MAX)

#endif