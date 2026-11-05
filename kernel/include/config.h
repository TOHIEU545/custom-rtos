#ifndef RTOS_CONFIG_H
#define RTOS_CONFIG_H

/**
 * @file config.h
 * @brief Cấu hình compile-time mặc định của MyRTOS (tick rate, giới hạn task/priority, cờ debug).
 */

/** @brief Số lần SysTick tick trong 1 giây. */
#define RTOS_TICK_RATE_HZ 1000U

/** @brief Số task tĩnh tối đa hệ thống hỗ trợ. */
#define RTOS_MAX_TASKS 9U

/** @brief Số mức priority hợp lệ (0..RTOS_MAX_PRIORITIES-1, số càng lớn càng ưu tiên). */
#define RTOS_MAX_PRIORITIES 8U

/** @brief Kích thước stack (theo word) của idle task. */
#define RTOS_IDLE_STACK_WORDS 128U

/** @brief Bật/tắt fill pattern 0xA5 lên stack khi tạo task (phục vụ đo high-water mark). */
#define RTOS_ENABLE_STACK_FILL 1

/** @brief Bật/tắt guard word phát hiện stack overflow. */
#define RTOS_ENABLE_STACK_GUARD 1

#endif
