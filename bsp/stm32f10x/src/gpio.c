#include "gpio.h"

#include "stm32f10x.h"

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
        RCC->APB2ENR |= RCC_APB2ENR_IOPAEN;
        break;
    case BSP_GPIO_PORT_B:
        RCC->APB2ENR |= RCC_APB2ENR_IOPBEN;
        break;
    case BSP_GPIO_PORT_C:
        RCC->APB2ENR |= RCC_APB2ENR_IOPCEN;
        break;
    case BSP_GPIO_PORT_D:
        RCC->APB2ENR |= RCC_APB2ENR_IOPDEN;
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
        mode_bits = 0x3U; /* output, toc do 50 MHz */
        cnf_bits = 0x0U;  /* push-pull */
        break;
    case BSP_GPIO_MODE_INPUT_PULLUP:
        mode_bits = 0x0U;
        cnf_bits = 0x2U; /* input voi pull-up/pull-down, chon huong qua ODR */
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
