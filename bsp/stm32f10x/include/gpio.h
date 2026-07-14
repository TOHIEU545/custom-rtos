#ifndef BSP_GPIO_H
#define BSP_GPIO_H
/**
 * @file gpio.h
 * @brief Driver GPIO tổng quát: config/set/reset/toggle/read theo port + pin.
 */
#include <stdbool.h>
#include <stdint.h>

/** @brief Port GPIO. */
typedef enum
{
    BSP_GPIO_PORT_A = 0,
    BSP_GPIO_PORT_B,
    BSP_GPIO_PORT_C,
    BSP_GPIO_PORT_D,
} bsp_gpio_port_t;

/** @brief Chế độ chân GPIO. */
typedef enum
{
    BSP_GPIO_MODE_INPUT_FLOATING,
    BSP_GPIO_MODE_INPUT_PULLUP,
    BSP_GPIO_MODE_INPUT_PULLDOWN,
    BSP_GPIO_MODE_OUTPUT_PUSH_PULL,
} bsp_gpio_mode_t;

/**
 * @brief Cấu hình một chân GPIO (tự bật clock cho port nếu chưa bật).
 * @param port Cổng GPIO.
 * @param pin Số chân (0-15).
 * @param mode Chế độ.
 */
void bsp_gpio_config(bsp_gpio_port_t port, uint8_t pin, bsp_gpio_mode_t mode);

/** @brief Set (qua BSRR, atomic). */
void bsp_gpio_set(bsp_gpio_port_t port, uint8_t pin);

/** @brief Reset (qua BSRR, atomic). */
void bsp_gpio_reset(bsp_gpio_port_t port, uint8_t pin);

/** @brief Đảo trạng thái chân hiện tại (đọc-sửa-ghi ODR). */
void bsp_gpio_toggle(bsp_gpio_port_t port, uint8_t pin);

/** @brief Đọc (qua IDR). */
bool bsp_gpio_read(bsp_gpio_port_t port, uint8_t pin);
#endif
