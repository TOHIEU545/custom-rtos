#include "led.h"

#include "stm32f10x_gpio.h"
#include "stm32f10x_rcc.h"

void led_init(void)
{
    GPIO_InitTypeDef gpio;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

    gpio.GPIO_Pin = GPIO_Pin_8;
    gpio.GPIO_Mode = GPIO_Mode_Out_PP;
    gpio.GPIO_Speed = GPIO_Speed_2MHz;

    GPIO_Init(GPIOA, &gpio);
}

void led_toggle(void)
{
    GPIO_WriteBit(GPIOA, GPIO_Pin_8,
                  (BitAction)(1 - GPIO_ReadOutputDataBit(GPIOA, GPIO_Pin_8)));
}
