#ifndef BSP_RCC_H
#define BSP_RCC_H
/**
 * @file rcc.h
 * @brief Helper enable clock cho từng ngoại vi qua RCC (APB1/APB2 enable bit).
 * @note Không phải system clock setup - phần đó (HSE/PLL/SystemCoreClock) do
 * vendor SystemInit()/SystemCoreClockUpdate() trong system/system_stm32f10x.c
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
#endif
