#ifndef BSP_H
#define BSP_H

/** @file bsp.h
 *  @brief Board support and peripheral helpers for STM32F103.
 */
#include <stdbool.h>
#include <stddef.h>
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
    BSP_GPIO_MODE_OUTPUT_ALT_PUSH_PULL, /**< Dùng cho chân peripheral (UART TX,...). */
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

/**
 * @brief Helper enable clock cho từng ngoại vi qua RCC (APB1/APB2 enable bit).
 * @note Không phải system clock setup - phần đó (HSE/PLL/SystemCoreClock) do
 * vendor SystemInit()/SystemCoreClockUpdate() trong
 * vendor/ST/STM32F10x/Source/system_stm32f10x.c
 * đảm nhiệm, không lặp lại ở đây.
 */

/** @brief Ngoại vi có thể enable clock qua bsp_rcc_enable(). */
typedef enum
{
    BSP_RCC_PERIPH_GPIOA,
    BSP_RCC_PERIPH_GPIOB,
    BSP_RCC_PERIPH_GPIOC,
    BSP_RCC_PERIPH_GPIOD,
    BSP_RCC_PERIPH_USART1,
    BSP_RCC_PERIPH_AFIO,
} bsp_rcc_periph_t;

/**
 * @brief Bật clock cho một ngoại vi (idempotent - gọi nhiều lần không sao).
 * @param periph Ngoại vi cần bật clock.
 */
void bsp_rcc_enable(bsp_rcc_periph_t periph);

/* USART1 polling driver (PA9=TX, PA10=RX, no remap). */
/**
 * @brief Init USART1: 8 bit, không parity, 1 stop bit, TX+RX enable.
 * @param baudrate Tốc độ baud mong muốn, ví dụ 115200U.
 */
void bsp_uart_init(uint32_t baudrate);

/** @brief Gửi 1 byte, polling chờ cờ TXE. */
void bsp_uart_write_byte(uint8_t byte);

/** @brief Gửi buffer, polling từng byte. */
void bsp_uart_write(const void *data, size_t size);

/** @brief Gửi C-string (không gửi ký tự NUL), polling. */
void bsp_uart_write_string(const char *string);

/* EXTI configuration for GPIO inputs. */

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
 *       thể request context switch (`rtos_port_request_context_switch()`) mà
 *       không bị PendSV chen ngang giữa chừng.
 */
void bsp_exti_config(bsp_gpio_port_t port, uint8_t pin, bsp_exti_trigger_t trigger);

/** @brief true nếu EXTI line ứng với pin đang pending. */
bool bsp_exti_pending(uint8_t pin);

/** @brief Xoá pending bit của EXTI line ứng với pin (write-1-to-clear, phải gọi trong ISR). */
void bsp_exti_clear_pending(uint8_t pin);

/* Board-level pin mapping and helpers. */

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

/** @brief Tần số clock lõi hiện tại, dùng để cấu hình RTOS tick. */
uint32_t board_cpu_clock_hz(void);

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
