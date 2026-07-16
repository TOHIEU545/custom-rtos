#include "stm32f10x.h"
#include <rcc.h>

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
    default:
        break;
    }
}
