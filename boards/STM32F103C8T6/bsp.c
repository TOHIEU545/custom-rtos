#include "stm32f10x.h"
#include "bsp.h"

void bsp_rcc_enable(bsp_rcc_periph_t periph)
{
    switch (periph)
    {
    case BSP_RCC_PERIPH_GPIOA:
        RCC->APB2ENR |= RCC_APB2ENR_IOPAEN;
        break;
    case BSP_RCC_PERIPH_GPIOB:
        RCC->APB2ENR |= RCC_APB2ENR_IOPBEN;
        break;
    case BSP_RCC_PERIPH_GPIOC:
        RCC->APB2ENR |= RCC_APB2ENR_IOPCEN;
        break;
    case BSP_RCC_PERIPH_GPIOD:
        RCC->APB2ENR |= RCC_APB2ENR_IOPDEN;
        break;
    case BSP_RCC_PERIPH_USART1:
        RCC->APB2ENR |= RCC_APB2ENR_USART1EN;
        break;
    case BSP_RCC_PERIPH_AFIO:
        RCC->APB2ENR |= RCC_APB2ENR_AFIOEN;
        break;
    default:
        break;
    }
}
static GPIO_TypeDef *gpio_port_reg(bsp_gpio_port_t port)
{
    switch (port)
    {
    case BSP_GPIO_PORT_A:
        return GPIOA;
    case BSP_GPIO_PORT_B:
        return GPIOB;
    case BSP_GPIO_PORT_C:
        return GPIOC;
    case BSP_GPIO_PORT_D:
        return GPIOD;
    default:
        return GPIOA;
    }
}

static void gpio_clock_enable(bsp_gpio_port_t port)
{
    switch (port)
    {
    case BSP_GPIO_PORT_A:
        bsp_rcc_enable(BSP_RCC_PERIPH_GPIOA);
        break;
    case BSP_GPIO_PORT_B:
        bsp_rcc_enable(BSP_RCC_PERIPH_GPIOB);
        break;
    case BSP_GPIO_PORT_C:
        bsp_rcc_enable(BSP_RCC_PERIPH_GPIOC);
        break;
    case BSP_GPIO_PORT_D:
        bsp_rcc_enable(BSP_RCC_PERIPH_GPIOD);
        break;
    default:
        break;
    }
}

void bsp_gpio_config(bsp_gpio_port_t port, uint8_t pin, bsp_gpio_mode_t mode)
{
    GPIO_TypeDef *reg = gpio_port_reg(port);
    volatile uint32_t *cr = (pin < 8U) ? &reg->CRL : &reg->CRH;
    uint32_t position = (uint32_t)(pin & 0x07U) * 4U;
    uint32_t mode_bits;
    uint32_t cnf_bits;

    gpio_clock_enable(port);

    switch (mode)
    {
    case BSP_GPIO_MODE_OUTPUT_PUSH_PULL:
        mode_bits = 0x3U; /* output, tốc độ 50 MHz */
        cnf_bits = 0x0U;  /* push-pull */
        break;
    case BSP_GPIO_MODE_OUTPUT_ALT_PUSH_PULL:
        mode_bits = 0x3U; /* output, tốc độ 50 MHz */
        cnf_bits = 0x2U;  /* alternate function push-pull */
        break;
    case BSP_GPIO_MODE_INPUT_PULLUP:
        mode_bits = 0x0U;
        cnf_bits = 0x2U; /* input với pull-up/pull-down, chọn hướng qua ODR */
        reg->ODR |= (1U << pin);
        break;
    case BSP_GPIO_MODE_INPUT_PULLDOWN:
        mode_bits = 0x0U;
        cnf_bits = 0x2U;
        reg->ODR &= ~(1U << pin);
        break;
    case BSP_GPIO_MODE_INPUT_FLOATING:
    default:
        mode_bits = 0x0U;
        cnf_bits = 0x1U;
        break;
    }

    *cr &= ~(0xFU << position);
    *cr |= ((cnf_bits << 2U) | mode_bits) << position;
}

void bsp_gpio_set(bsp_gpio_port_t port, uint8_t pin)
{
    gpio_port_reg(port)->BSRR = (1U << pin);
}

void bsp_gpio_reset(bsp_gpio_port_t port, uint8_t pin)
{
    gpio_port_reg(port)->BSRR = (1U << (pin + 16U));
}

void bsp_gpio_toggle(bsp_gpio_port_t port, uint8_t pin)
{
    GPIO_TypeDef *reg = gpio_port_reg(port);
    reg->ODR ^= (1U << pin);
}

bool bsp_gpio_read(bsp_gpio_port_t port, uint8_t pin)
{
    return (gpio_port_reg(port)->IDR & (1U << pin)) != 0U;
}

#define UART_TX_PORT BSP_GPIO_PORT_A
#define UART_TX_PIN 9U
#define UART_RX_PORT BSP_GPIO_PORT_A
#define UART_RX_PIN 10U

extern uint32_t SystemCoreClock;

void bsp_uart_init(uint32_t baudrate)
{
    /* USART1 nằm trên APB2. Giả định APB2 prescaler = DIV1 (đúng theo
     * SetSysClockTo72() trong vendor system source) nên PCLK2 == SystemCoreClock.
     * Nếu đổi prescaler APB2 sau này phải tính lại PCLK2 cho đúng. */
    uint32_t pclk2 = SystemCoreClock;
    uint32_t usartdiv_x100 = (pclk2 * 25U) / (4U * baudrate);
    uint32_t mantissa = usartdiv_x100 / 100U;
    uint32_t fraction = ((usartdiv_x100 - (mantissa * 100U)) * 16U + 50U) / 100U;

    bsp_rcc_enable(BSP_RCC_PERIPH_USART1);

    bsp_gpio_config(UART_TX_PORT, UART_TX_PIN, BSP_GPIO_MODE_OUTPUT_ALT_PUSH_PULL);
    bsp_gpio_config(UART_RX_PORT, UART_RX_PIN, BSP_GPIO_MODE_INPUT_FLOATING);

    USART1->BRR = (uint16_t)((mantissa << 4U) | (fraction & 0xFU));
    USART1->CR1 = (uint16_t)(USART_CR1_TE | USART_CR1_RE | USART_CR1_UE);
}

void bsp_uart_write_byte(uint8_t byte)
{
    while ((USART1->SR & USART_SR_TXE) == 0U)
    {
    }
    USART1->DR = byte;
}

void bsp_uart_write(const void *data, size_t size)
{
    const uint8_t *p = data;
    for (size_t i = 0; i < size; i++)
    {
        bsp_uart_write_byte(p[i]);
    }
}

void bsp_uart_write_string(const char *s)
{
    if (s == NULL)
    {
        return;
    }
    while (*s)
    {
        bsp_uart_write_byte((uint8_t)*s++);
    }
}

/* AFIO->EXTICR[n] chia 16 EXTI line thành 4 thanh ghi, mỗi line chiếm 4 bit -
 * xem RM0008 mục AFIO_EXTICRx. */
static uint32_t exti_afio_cr_index(uint8_t pin)
{
    return (uint32_t)(pin >> 2U);
}

static uint32_t exti_afio_cr_shift(uint8_t pin)
{
    return (uint32_t)(pin & 0x03U) * 4U;
}

static uint32_t exti_afio_port_code(bsp_gpio_port_t port)
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

/* EXTI0-4 có vector NVIC riêng; EXTI5-9 và EXTI10-15 dùng chung một vector. */
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
    uint32_t cr_index = exti_afio_cr_index(pin);
    uint32_t shift = exti_afio_cr_shift(pin);
    uint32_t line_mask = 1UL << pin;

    bsp_rcc_enable(BSP_RCC_PERIPH_AFIO);

    AFIO->EXTICR[cr_index] &= ~(0xFUL << shift);
    AFIO->EXTICR[cr_index] |= (exti_afio_port_code(port) << shift);

    switch (trigger)
    {
    case BSP_EXTI_TRIGGER_RISING:
        EXTI->RTSR |= line_mask;
        EXTI->FTSR &= ~line_mask;
        break;
    case BSP_EXTI_TRIGGER_BOTH:
        EXTI->RTSR |= line_mask;
        EXTI->FTSR |= line_mask;
        break;
    case BSP_EXTI_TRIGGER_FALLING:
    default:
        EXTI->RTSR &= ~line_mask;
        EXTI->FTSR |= line_mask;
        break;
    }

    EXTI->PR = line_mask; /* clear pending bit rác trước khi unmask */
    EXTI->IMR |= line_mask;

    NVIC_EnableIRQ(exti_irqn(pin));
}

bool bsp_exti_pending(uint8_t pin)
{
    return (EXTI->PR & (1UL << pin)) != 0U;
}

void bsp_exti_clear_pending(uint8_t pin)
{
    EXTI->PR = (1UL << pin); /* write-1-to-clear theo hardware */
}

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

uint32_t board_cpu_clock_hz(void)
{
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
    bsp_gpio_port_t port;
    uint8_t pin;

    board_key_port_pin(key, &port, &pin);

    /* Phần cứng active-low: đọc 0 khi nhấn, nên đảo lại để API trả về đúng nghĩa "pressed". */
    return !bsp_gpio_read(port, pin);
}
