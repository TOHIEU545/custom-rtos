#ifndef NTOS_CONFIG_DEFAULT_H
#define NTOS_CONFIG_DEFAULT_H
/**
 * @file ntos_config_default.h
 * @brief Cấu hình compile-time mặc định của NTOS (tick rate, giới hạn task/priority, cờ debug).
 */

/** @brief Số lần SysTick tick trong 1 giây. */
#define NTOS_TICK_RATE_HZ 1000U

/** @brief Số task tĩnh tối đa hệ thống hỗ trợ. */
#define NTOS_MAX_TASKS 9U

/** @brief Số mức priority hợp lệ (0..NTOS_MAX_PRIORITIES-1, số càng lớn càng ưu tiên). */
#define NTOS_MAX_PRIORITIES 8U

/** @brief Kích thước stack (theo word) của idle task. */
#define NTOS_IDLE_STACK_WORDS 128U

/** @brief Bật/tắt NTOS_ASSERT trong toàn bộ kernel. */
#define NTOS_ENABLE_ASSERT 1

/** @brief Bật/tắt fill pattern 0xA5 lên stack khi tạo task (phục vụ đo high-water mark). */
#define NTOS_ENABLE_STACK_FILL 1

/** @brief Bật/tắt guard word phát hiện stack overflow. */
#define NTOS_ENABLE_STACK_GUARD 1

#endif
