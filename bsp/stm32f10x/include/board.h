#ifndef BOARD_H
#define BOARD_H
/**
 * @file board.h
 * @brief Board mapping (LED, UART,...) và API board-level. Không chứa register-level.
 */
#include <gpio.h>
#include <stdbool.h>
#include <stdint.h>

/** @brief LED D0: GPIO port A, pin 8. */
#define BOARD_LED_D0_PORT BSP_GPIO_PORT_A
#define BOARD_LED_D0_PIN 8U

/** @brief LED D1: GPIO port D, pin 2. */
#define BOARD_LED_D1_PORT BSP_GPIO_PORT_D
#define BOARD_LED_D1_PIN 2U

/** @brief Baud rate mặc định cho UART1 của board. */
#define BOARD_UART_BAUDRATE 115200U

/** @brief Key1: GPIO port C, pin 13. Active-low (đọc 0 khi nhấn). */
#define BOARD_KEY1_PORT BSP_GPIO_PORT_C
#define BOARD_KEY1_PIN 13U

/** @brief Key0: GPIO port C, pin 1. Active-low (đọc 0 khi nhấn). */
#define BOARD_KEY0_PORT BSP_GPIO_PORT_C
#define BOARD_KEY0_PIN 1U

/** @brief LED trên board. */
typedef enum
{
    BOARD_LED_D0,
    BOARD_LED_D1,
} board_led_t;

/** @brief Nút nhấn trên board. */
typedef enum
{
    BOARD_KEY0,
    BOARD_KEY1,
} board_key_t;

/** @brief Cấu hình các chân/ngoại vi cố định của board (2 LED, 2 nút nhấn, UART1). */
void board_init(void);

/** @brief Bật/tắt LED chỉ định. */
void board_led_set(board_led_t led, bool on);

/** @brief Đảo trạng thái LED chỉ định. */
void board_led_toggle(board_led_t led);

/**
 * @brief Đọc trạng thái nút nhấn.
 * @return true nếu đang được nhấn (đã đảo ngược mức active-low của phần cứng).
 */
bool board_key_pressed(board_key_t key);
#endif
