#include <exti.h>

#include <rcc.h>
#include "stm32f10x.h"

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
