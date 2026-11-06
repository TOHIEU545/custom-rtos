#include <rtos.h>
#include <bsp.h>

#include <stddef.h>
#include <stdint.h>

#define BLINK_STACK_WORDS ((size_t)64U)
#define BLINK_PRIORITY ((rtos_priority_t)1U)
#define BLINK_PERIOD_TICKS ((rtos_tick_t)500U)

static rtos_tcb_t blink_tcb;
static _Alignas(8) uint32_t blink_stack[BLINK_STACK_WORDS];

static void blink_task(void *argument)
{
    (void)argument;

    for (;;)
    {
        board_led_toggle(BOARD_LED_D0);
        rtos_delay(BLINK_PERIOD_TICKS);
    }
}

int main(void)
{
    rtos_status_t status;

    board_init();
    board_led_set(BOARD_LED_D0, false);
    rtos_init();

    status = rtos_task_create_static(&blink_tcb, "blink", blink_task, NULL, blink_stack,
                                     BLINK_STACK_WORDS, BLINK_PRIORITY);
    if (status != RTOS_OK)
    {
        for (;;)
        {
        }
    }

    rtos_start(&blink_tcb, board_cpu_clock_hz());

    for (;;)
    {
    }
}
