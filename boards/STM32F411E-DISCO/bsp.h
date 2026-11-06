#ifndef BSP_H
#define BSP_H

/** @file bsp.h
 *  @brief Board support and peripheral helpers for STM32F411E-DISCO.
 */
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/** @brief GPIO ports currently exposed by the BSP. */
typedef enum
{
    BSP_GPIO_PORT_A = 0,
    BSP_GPIO_PORT_B,
    BSP_GPIO_PORT_C,
    BSP_GPIO_PORT_D,
} bsp_gpio_port_t;

/** @brief GPIO pin modes used by this board. */
typedef enum
{
    BSP_GPIO_MODE_INPUT_FLOATING,
    BSP_GPIO_MODE_INPUT_PULLUP,
    BSP_GPIO_MODE_INPUT_PULLDOWN,
    BSP_GPIO_MODE_OUTPUT_PUSH_PULL,
    BSP_GPIO_MODE_OUTPUT_ALT_PUSH_PULL,
} bsp_gpio_mode_t;

void bsp_gpio_config(bsp_gpio_port_t port, uint8_t pin, bsp_gpio_mode_t mode);
void bsp_gpio_set(bsp_gpio_port_t port, uint8_t pin);
void bsp_gpio_reset(bsp_gpio_port_t port, uint8_t pin);
void bsp_gpio_toggle(bsp_gpio_port_t port, uint8_t pin);
bool bsp_gpio_read(bsp_gpio_port_t port, uint8_t pin);

/** @brief Peripherals whose clocks can be enabled through the BSP. */
typedef enum
{
    BSP_RCC_PERIPH_GPIOA,
    BSP_RCC_PERIPH_GPIOB,
    BSP_RCC_PERIPH_GPIOC,
    BSP_RCC_PERIPH_GPIOD,
    BSP_RCC_PERIPH_USART2,
    BSP_RCC_PERIPH_SYSCFG,
} bsp_rcc_periph_t;

void bsp_rcc_enable(bsp_rcc_periph_t periph);

/* USART2 polling driver: PA2=TX connects to adapter RX, PA3=RX connects to
 * adapter TX. Both pins use alternate function AF7. */
void bsp_uart_init(uint32_t baudrate);
void bsp_uart_write_byte(uint8_t byte);
void bsp_uart_write(const void *data, size_t size);
void bsp_uart_write_string(const char *string);

typedef enum
{
    BSP_EXTI_TRIGGER_FALLING,
    BSP_EXTI_TRIGGER_RISING,
    BSP_EXTI_TRIGGER_BOTH,
} bsp_exti_trigger_t;

void bsp_exti_config(bsp_gpio_port_t port, uint8_t pin, bsp_exti_trigger_t trigger);
bool bsp_exti_pending(uint8_t pin);
void bsp_exti_clear_pending(uint8_t pin);

/* Board-level LED mapping. All four LEDs are active-high.
 *
 *   Board name    Color       GPIO
 *   LD3           Orange      PD13
 *   LD4           Green       PD12
 *   LD5           Red         PD14
 *   LD6           Blue        PD15
 */
#define BOARD_LED_LD3_PORT BSP_GPIO_PORT_D
#define BOARD_LED_LD3_PIN 13U

#define BOARD_LED_LD4_PORT BSP_GPIO_PORT_D
#define BOARD_LED_LD4_PIN 12U

#define BOARD_LED_LD5_PORT BSP_GPIO_PORT_D
#define BOARD_LED_LD5_PIN 14U

#define BOARD_LED_LD6_PORT BSP_GPIO_PORT_D
#define BOARD_LED_LD6_PIN 15U

/* B1 USER button: external board circuitry keeps PA0 low when released and
 * drives it high while pressed. */
#define BOARD_KEY_B1_PORT BSP_GPIO_PORT_A
#define BOARD_KEY_B1_PIN 0U

#define BOARD_UART_TX_PORT BSP_GPIO_PORT_A
#define BOARD_UART_TX_PIN 2U
#define BOARD_UART_RX_PORT BSP_GPIO_PORT_A
#define BOARD_UART_RX_PIN 3U
#define BOARD_UART_BAUDRATE 115200U

typedef enum
{
    BOARD_LED_LD3_ORANGE,
    BOARD_LED_LD4_GREEN,
    BOARD_LED_LD5_RED,
    BOARD_LED_LD6_BLUE,
} board_led_t;

/* Color-first aliases for application code that does not care about the
 * silkscreen names printed on the board. */
#define BOARD_LED_ORANGE BOARD_LED_LD3_ORANGE
#define BOARD_LED_GREEN BOARD_LED_LD4_GREEN
#define BOARD_LED_RED BOARD_LED_LD5_RED
#define BOARD_LED_BLUE BOARD_LED_LD6_BLUE

typedef enum
{
    BOARD_KEY_B1,
} board_key_t;

/** @brief Initialize the four LEDs, B1 button and USART2. */
void board_init(void);

/** @brief Current Cortex-M4 HCLK frequency used to configure the RTOS tick. */
uint32_t board_cpu_clock_hz(void);

void board_led_set(board_led_t led, bool on);
void board_led_toggle(board_led_t led);

/** @return true while the active-high B1 USER button is pressed. */
bool board_key_pressed(board_key_t key);

#endif
