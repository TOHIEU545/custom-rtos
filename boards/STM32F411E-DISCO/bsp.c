#include "stm32f4xx.h"
#include "bsp.h"

#define USART2_ALTERNATE_FUNCTION 7U

static GPIO_TypeDef *gpio_port_reg(bsp_gpio_port_t port)
{
    switch (port)
    {
    case BSP_GPIO_PORT_B:
        return GPIOB;
    case BSP_GPIO_PORT_C:
        return GPIOC;
    case BSP_GPIO_PORT_D:
        return GPIOD;
    case BSP_GPIO_PORT_A:
    default:
        return GPIOA;
    }
}

void bsp_rcc_enable(bsp_rcc_periph_t periph)
{
    switch (periph)
    {
    case BSP_RCC_PERIPH_GPIOA:
        RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
        (void)RCC->AHB1ENR;
        break;
    case BSP_RCC_PERIPH_GPIOB:
        RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN;
        (void)RCC->AHB1ENR;
        break;
    case BSP_RCC_PERIPH_GPIOC:
        RCC->AHB1ENR |= RCC_AHB1ENR_GPIOCEN;
        (void)RCC->AHB1ENR;
        break;
    case BSP_RCC_PERIPH_GPIOD:
        RCC->AHB1ENR |= RCC_AHB1ENR_GPIODEN;
        (void)RCC->AHB1ENR;
        break;
    case BSP_RCC_PERIPH_USART2:
        RCC->APB1ENR |= RCC_APB1ENR_USART2EN;
        (void)RCC->APB1ENR;
        break;
    case BSP_RCC_PERIPH_SYSCFG:
        RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;
        (void)RCC->APB2ENR;
        break;
    default:
        break;
    }
}

static void gpio_clock_enable(bsp_gpio_port_t port)
{
    switch (port)
    {
    case BSP_GPIO_PORT_B:
        bsp_rcc_enable(BSP_RCC_PERIPH_GPIOB);
        break;
    case BSP_GPIO_PORT_C:
        bsp_rcc_enable(BSP_RCC_PERIPH_GPIOC);
        break;
    case BSP_GPIO_PORT_D:
        bsp_rcc_enable(BSP_RCC_PERIPH_GPIOD);
        break;
    case BSP_GPIO_PORT_A:
    default:
        bsp_rcc_enable(BSP_RCC_PERIPH_GPIOA);
        break;
    }
}

void bsp_gpio_config(bsp_gpio_port_t port, uint8_t pin, bsp_gpio_mode_t mode)
{
    GPIO_TypeDef *reg = gpio_port_reg(port);
    uint32_t shift = (uint32_t)pin * 2U;
    uint32_t moder = 0U;
    uint32_t pupdr = 0U;

    gpio_clock_enable(port);

    switch (mode)
    {
    case BSP_GPIO_MODE_INPUT_PULLUP:
        pupdr = 1U;
        break;
    case BSP_GPIO_MODE_INPUT_PULLDOWN:
        pupdr = 2U;
        break;
    case BSP_GPIO_MODE_OUTPUT_PUSH_PULL:
        moder = 1U;
        break;
    case BSP_GPIO_MODE_OUTPUT_ALT_PUSH_PULL:
        moder = 2U;
        break;
    case BSP_GPIO_MODE_INPUT_FLOATING:
    default:
        break;
    }

    reg->MODER = (reg->MODER & ~(3UL << shift)) | (moder << shift);
    reg->PUPDR = (reg->PUPDR & ~(3UL << shift)) | (pupdr << shift);

    if ((mode == BSP_GPIO_MODE_OUTPUT_PUSH_PULL) ||
        (mode == BSP_GPIO_MODE_OUTPUT_ALT_PUSH_PULL))
    {
        reg->OTYPER &= ~(1UL << pin);
        reg->OSPEEDR = (reg->OSPEEDR & ~(3UL << shift)) | (2UL << shift);
    }
}

void bsp_gpio_set(bsp_gpio_port_t port, uint8_t pin)
{
    gpio_port_reg(port)->BSRR = (1UL << pin);
}

void bsp_gpio_reset(bsp_gpio_port_t port, uint8_t pin)
{
    gpio_port_reg(port)->BSRR = (1UL << (pin + 16U));
}

void bsp_gpio_toggle(bsp_gpio_port_t port, uint8_t pin)
{
    gpio_port_reg(port)->ODR ^= (1UL << pin);
}

bool bsp_gpio_read(bsp_gpio_port_t port, uint8_t pin)
{
    return (gpio_port_reg(port)->IDR & (1UL << pin)) != 0U;
}

static void gpio_set_alternate_function(bsp_gpio_port_t port, uint8_t pin, uint8_t function)
{
    GPIO_TypeDef *reg = gpio_port_reg(port);
    uint32_t index = (uint32_t)pin >> 3U;
    uint32_t shift = ((uint32_t)pin & 0x07U) * 4U;

    reg->AFR[index] = (reg->AFR[index] & ~(0xFUL << shift)) |
                      ((uint32_t)(function & 0x0FU) << shift);
}

static uint32_t pclk1_hz(void)
{
    static const uint8_t divisor[8] = {1U, 1U, 1U, 1U, 2U, 4U, 8U, 16U};
    uint32_t ppre1;

    SystemCoreClockUpdate();
    ppre1 = (RCC->CFGR & RCC_CFGR_PPRE1) >> RCC_CFGR_PPRE1_Pos;

    return SystemCoreClock / divisor[ppre1];
}

void bsp_uart_init(uint32_t baudrate)
{
    uint32_t brr;

    bsp_rcc_enable(BSP_RCC_PERIPH_USART2);

    bsp_gpio_config(BOARD_UART_TX_PORT, BOARD_UART_TX_PIN,
                    BSP_GPIO_MODE_OUTPUT_ALT_PUSH_PULL);
    bsp_gpio_config(BOARD_UART_RX_PORT, BOARD_UART_RX_PIN,
                    BSP_GPIO_MODE_OUTPUT_ALT_PUSH_PULL);
    gpio_set_alternate_function(BOARD_UART_TX_PORT, BOARD_UART_TX_PIN,
                                USART2_ALTERNATE_FUNCTION);
    gpio_set_alternate_function(BOARD_UART_RX_PORT, BOARD_UART_RX_PIN,
                                USART2_ALTERNATE_FUNCTION);

    brr = (pclk1_hz() + (baudrate / 2U)) / baudrate;

    USART2->CR1 = 0U;
    USART2->CR2 = 0U;
    USART2->CR3 = 0U;
    USART2->BRR = brr;
    USART2->CR1 = USART_CR1_TE | USART_CR1_RE | USART_CR1_UE;
}

void bsp_uart_write_byte(uint8_t byte)
{
    while ((USART2->SR & USART_SR_TXE) == 0U)
    {
    }

    USART2->DR = byte;
}

void bsp_uart_write(const void *data, size_t size)
{
    const uint8_t *bytes = data;
    size_t index;

    for (index = 0U; index < size; index++)
    {
        bsp_uart_write_byte(bytes[index]);
    }
}

void bsp_uart_write_string(const char *string)
{
    if (string == NULL)
    {
        return;
    }

    while (*string != '\0')
    {
        bsp_uart_write_byte((uint8_t)*string++);
    }
}

static uint32_t exti_port_code(bsp_gpio_port_t port)
{
    switch (port)
    {
    case BSP_GPIO_PORT_B:
        return 1U;
    case BSP_GPIO_PORT_C:
        return 2U;
    case BSP_GPIO_PORT_D:
        return 3U;
    case BSP_GPIO_PORT_A:
    default:
        return 0U;
    }
}

static IRQn_Type exti_irqn(uint8_t pin)
{
    switch (pin)
    {
    case 0U:
        return EXTI0_IRQn;
    case 1U:
        return EXTI1_IRQn;
    case 2U:
        return EXTI2_IRQn;
    case 3U:
        return EXTI3_IRQn;
    case 4U:
        return EXTI4_IRQn;
    default:
        return (pin < 10U) ? EXTI9_5_IRQn : EXTI15_10_IRQn;
    }
}

void bsp_exti_config(bsp_gpio_port_t port, uint8_t pin, bsp_exti_trigger_t trigger)
{
    uint32_t index = (uint32_t)pin >> 2U;
    uint32_t shift = ((uint32_t)pin & 0x03U) * 4U;
    uint32_t line = 1UL << pin;

    bsp_rcc_enable(BSP_RCC_PERIPH_SYSCFG);

    SYSCFG->EXTICR[index] = (SYSCFG->EXTICR[index] & ~(0xFUL << shift)) |
                            (exti_port_code(port) << shift);

    switch (trigger)
    {
    case BSP_EXTI_TRIGGER_RISING:
        EXTI->RTSR |= line;
        EXTI->FTSR &= ~line;
        break;
    case BSP_EXTI_TRIGGER_BOTH:
        EXTI->RTSR |= line;
        EXTI->FTSR |= line;
        break;
    case BSP_EXTI_TRIGGER_FALLING:
    default:
        EXTI->RTSR &= ~line;
        EXTI->FTSR |= line;
        break;
    }

    EXTI->PR = line;
    EXTI->IMR |= line;
    NVIC_EnableIRQ(exti_irqn(pin));
}

bool bsp_exti_pending(uint8_t pin)
{
    return (EXTI->PR & (1UL << pin)) != 0U;
}

void bsp_exti_clear_pending(uint8_t pin)
{
    EXTI->PR = (1UL << pin);
}

static void board_led_port_pin(board_led_t led, bsp_gpio_port_t *port, uint8_t *pin)
{
    switch (led)
    {
    case BOARD_LED_LD3_ORANGE:
        *port = BOARD_LED_LD3_PORT;
        *pin = BOARD_LED_LD3_PIN;
        break;
    case BOARD_LED_LD5_RED:
        *port = BOARD_LED_LD5_PORT;
        *pin = BOARD_LED_LD5_PIN;
        break;
    case BOARD_LED_LD6_BLUE:
        *port = BOARD_LED_LD6_PORT;
        *pin = BOARD_LED_LD6_PIN;
        break;
    case BOARD_LED_LD4_GREEN:
    default:
        *port = BOARD_LED_LD4_PORT;
        *pin = BOARD_LED_LD4_PIN;
        break;
    }
}

void board_init(void)
{
    SystemCoreClockUpdate();

    bsp_gpio_config(BOARD_LED_LD3_PORT, BOARD_LED_LD3_PIN,
                    BSP_GPIO_MODE_OUTPUT_PUSH_PULL);
    bsp_gpio_config(BOARD_LED_LD4_PORT, BOARD_LED_LD4_PIN,
                    BSP_GPIO_MODE_OUTPUT_PUSH_PULL);
    bsp_gpio_config(BOARD_LED_LD5_PORT, BOARD_LED_LD5_PIN,
                    BSP_GPIO_MODE_OUTPUT_PUSH_PULL);
    bsp_gpio_config(BOARD_LED_LD6_PORT, BOARD_LED_LD6_PIN,
                    BSP_GPIO_MODE_OUTPUT_PUSH_PULL);

    board_led_set(BOARD_LED_LD3_ORANGE, false);
    board_led_set(BOARD_LED_LD4_GREEN, false);
    board_led_set(BOARD_LED_LD5_RED, false);
    board_led_set(BOARD_LED_LD6_BLUE, false);

    bsp_gpio_config(BOARD_KEY_B1_PORT, BOARD_KEY_B1_PIN,
                    BSP_GPIO_MODE_INPUT_FLOATING);

    bsp_uart_init(BOARD_UART_BAUDRATE);
}

uint32_t board_cpu_clock_hz(void)
{
    SystemCoreClockUpdate();
    return SystemCoreClock;
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
    (void)key;
    return bsp_gpio_read(BOARD_KEY_B1_PORT, BOARD_KEY_B1_PIN);
}
