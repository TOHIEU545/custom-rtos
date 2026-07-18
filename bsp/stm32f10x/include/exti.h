#ifndef BSP_EXTI_H
#define BSP_EXTI_H
/**
 * @file exti.h
 * @brief Cấu hình EXTI line theo cạnh - dùng để tạo ngắt từ GPIO input (nút nhấn).
 */
#include <gpio.h>
#include <stdbool.h>
#include <stdint.h>

/** @brief Cạnh kích hoạt ngắt EXTI. */
typedef enum
{
    BSP_EXTI_TRIGGER_FALLING,
    BSP_EXTI_TRIGGER_RISING,
    BSP_EXTI_TRIGGER_BOTH,
} bsp_exti_trigger_t;

/**
 * @brief Map EXTI line (== số pin) tới port chỉ định, cấu hình cạnh kích hoạt,
 *        unmask line và enable đúng vector NVIC tương ứng.
 * @param port Cổng GPIO nguồn của EXTI line (AFIO chỉ cho line trỏ tới một
 *             port tại một thời điểm - line dùng chung giữa các port cùng số pin).
 * @param pin Số chân (0-15), đồng thời là số EXTI line - phải đã được
 *            `bsp_gpio_config()` làm input trước khi gọi hàm này.
 * @param trigger Cạnh kích hoạt ngắt.
 * @note Không cấu hình priority NVIC riêng - dùng priority reset mặc định
 *       (cao hơn PendSV/SysTick vốn đã bị hạ xuống thấp nhất), đủ để ISR có
 *       thể request context switch (`ntos_port_request_context_switch()`) mà
 *       không bị PendSV chen ngang giữa chừng.
 */
void bsp_exti_config(bsp_gpio_port_t port, uint8_t pin, bsp_exti_trigger_t trigger);

/** @brief true nếu EXTI line ứng với pin đang pending. */
bool bsp_exti_pending(uint8_t pin);

/** @brief Xoá pending bit của EXTI line ứng với pin (write-1-to-clear, phải gọi trong ISR). */
void bsp_exti_clear_pending(uint8_t pin);

#endif
