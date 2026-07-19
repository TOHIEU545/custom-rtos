/*
 * Fixed-priority scheduler: task_high always preempts task_low. task_low
 * only gets CPU time while task_high is BLOCKED in ntos_delay().
 */
#include <ntos/ntos.h>
#include <board.h>
#include <uart.h>
#include <stddef.h>
#include <stdint.h>

#define STACK_WORDS ((size_t)64U)
#define HIGH_PRIORITY ((ntos_priority_t)2U)
#define LOW_PRIORITY ((ntos_priority_t)1U)
#define HIGH_DELAY_TICKS 500U
#define LOW_PRINT_EVERY 300000U

static ntos_tcb_t task_high_tcb;
static ntos_tcb_t task_low_tcb;
static _Alignas(8) uint32_t task_high_stack[STACK_WORDS];
static _Alignas(8) uint32_t task_low_stack[STACK_WORDS];

static void task_high(void *argument)
{
    (void)argument;

    for (;;)
    {
        bsp_uart_write_string("[high] run\r\n");
        bsp_uart_write_string("[high] delay\r\n");
        ntos_delay(HIGH_DELAY_TICKS);
        bsp_uart_write_string("[high] wake\r\n");
    }
}

/* Priority thấp - chỉ được scheduler chọn trong lúc task_high đang BLOCKED. */
static void task_low(void *argument)
{
    uint32_t count = 0U;
    (void)argument;

    for (;;)
    {
        if ((++count % LOW_PRINT_EVERY) == 0U)
        {
            bsp_uart_write_string("[low] run\r\n");
        }
    }
}

int main(void)
{
    ntos_status_t status_high;
    ntos_status_t status_low;

    board_init();
    ntos_init();

    status_high = ntos_task_create_static(&task_high_tcb, "task_high", task_high, NULL,
        task_high_stack, STACK_WORDS, HIGH_PRIORITY);
    status_low = ntos_task_create_static(&task_low_tcb, "task_low", task_low, NULL,
        task_low_stack, STACK_WORDS, LOW_PRIORITY);

    if ((status_high != NTOS_OK) || (status_low != NTOS_OK))
    {
        for (;;)
        {
        }
    }

    ntos_start(&task_high_tcb);

    for (;;)
    {
    }
}
