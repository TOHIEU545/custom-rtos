#include <stdint.h>

#include "stm32f10x.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_rcc.h"

/*
 * Delay thô để kiểm tra firmware.
 * Chưa dùng SysTick vì bài này chỉ tập trung vào add_executable().
 */
static void delay(volatile uint32_t count)
{
    while (count > 0U)
    {
        __NOP();
        count--;
    }
}

int main(void)
{
    GPIO_InitTypeDef gpio_init = {0};

    /*
     * Cấp clock cho GPIOC.
     */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

    /*
     * Blue Pill thường nối LED onboard vào PC13.
     * LED PC13 hoạt động active-low:
     *   GPIO mức 0: LED sáng
     *   GPIO mức 1: LED tắt
     */
    gpio_init.GPIO_Pin   = GPIO_Pin_8;
    gpio_init.GPIO_Mode  = GPIO_Mode_Out_PP;
    gpio_init.GPIO_Speed = GPIO_Speed_2MHz;

    GPIO_Init(GPIOA, &gpio_init);

    /* Ban đầu tắt LED. */
    GPIO_SetBits(GPIOA, GPIO_Pin_8);

    while (1)
    {
        /* Bật LED. */
        GPIO_ResetBits(GPIOA, GPIO_Pin_8);
        delay(800000U);

        /* Tắt LED. */
        GPIO_SetBits(GPIOA, GPIO_Pin_8);
        delay(800000U);
    }
}