#include "led.h"
#include <stdint.h>

static void delay(uint32_t ms)
{
    volatile uint32_t i, j;
    for (i = 0; i < ms; i++)
    {
        for (j = 0; j < 0x2aff; j++)
        {

        }
    }
}

int main(void)
{
    led_init();

    while (1) {
        led_toggle();
        delay(80);
    }
}
