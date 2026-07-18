#include <uart.h>

#include "stm32f10x.h"
#include <gpio.h>
#include <rcc.h>

#define UART_TX_PORT BSP_GPIO_PORT_A
#define UART_TX_PIN 9U
#define UART_RX_PORT BSP_GPIO_PORT_A
#define UART_RX_PIN 10U

extern uint32_t SystemCoreClock;

void bsp_uart_init(uint32_t baudrate)
{
    /* USART1 nằm trên APB2. Giả định APB2 prescaler = DIV1 (đúng theo
     * SetSysClockTo72() trong system_stm32f10x.c) nên PCLK2 == SystemCoreClock.
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
