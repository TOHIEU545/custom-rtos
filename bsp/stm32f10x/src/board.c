#include <board.h>

#include <uart.h>

static void board_led_port_pin(board_led_t led, bsp_gpio_port_t *port, uint8_t *pin)
{
    switch (led)
    {
    case BOARD_LED_D1:
        *port = BOARD_LED_D1_PORT;
        *pin = BOARD_LED_D1_PIN;
        break;
    case BOARD_LED_D0:
    default:
        *port = BOARD_LED_D0_PORT;
        *pin = BOARD_LED_D0_PIN;
        break;
    }
}

static void board_key_port_pin(board_key_t key, bsp_gpio_port_t *port, uint8_t *pin)
{
    switch (key)
    {
    case BOARD_KEY0:
        *port = BOARD_KEY0_PORT;
        *pin = BOARD_KEY0_PIN;
        break;
    case BOARD_KEY1:
    default:
        *port = BOARD_KEY1_PORT;
        *pin = BOARD_KEY1_PIN;
        break;
    }
}

void board_init(void)
{
    bsp_gpio_config(BOARD_LED_D0_PORT, BOARD_LED_D0_PIN, BSP_GPIO_MODE_OUTPUT_PUSH_PULL);
    bsp_gpio_config(BOARD_LED_D1_PORT, BOARD_LED_D1_PIN, BSP_GPIO_MODE_OUTPUT_PUSH_PULL);
    bsp_gpio_config(BOARD_KEY1_PORT, BOARD_KEY1_PIN, BSP_GPIO_MODE_INPUT_PULLUP);
    bsp_gpio_config(BOARD_KEY0_PORT, BOARD_KEY0_PIN, BSP_GPIO_MODE_INPUT_PULLUP);
    bsp_uart_init(BOARD_UART_BAUDRATE);
}

void board_led_set(board_led_t led, bool on)
{
    bsp_gpio_port_t port;
    uint8_t pin;

    board_led_port_pin(led, &port, &pin);

    if (on)
    {
        bsp_gpio_set(port, pin);
    }
    else
    {
        bsp_gpio_reset(port, pin);
    }
}

void board_led_toggle(board_led_t led)
{
    bsp_gpio_port_t port;
    uint8_t pin;

    board_led_port_pin(led, &port, &pin);
    bsp_gpio_toggle(port, pin);
}

bool board_key_pressed(board_key_t key)
{
    bsp_gpio_port_t port;
    uint8_t pin;

    board_key_port_pin(key, &port, &pin);

    /* Phần cứng active-low: đọc 0 khi nhấn, nên đảo lại để API trả về đúng nghĩa "pressed". */
    return !bsp_gpio_read(port, pin);
}
