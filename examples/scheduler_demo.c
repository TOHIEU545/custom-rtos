#include <rtos.h>
#include <bsp.h>

#include <stddef.h>
#include <stdint.h>

#define STACK_WORDS ((size_t)64U)

#define HIGH_PRIORITY ((rtos_priority_t)2U)
#define MEDIUM_PRIORITY ((rtos_priority_t)1U)

#define HIGH_PERIOD_TICKS ((rtos_tick_t)1000U)
#define MEDIUM_PERIOD_TICKS ((rtos_tick_t)2000U)

static rtos_tcb_t high_tcb;
static rtos_tcb_t medium_tcb;

static _Alignas(8) uint32_t high_stack[STACK_WORDS];
static _Alignas(8) uint32_t medium_stack[STACK_WORDS];

static void high_task(void *argument)
{
    (void)argument;

    for (;;)
    {
        bsp_uart_write_string("[high] run\r\n");
        rtos_delay(HIGH_PERIOD_TICKS);
    }
}
static void medium_task(void *argument)
{
    (void)argument;

    for (;;)
    {
        bsp_uart_write_string("[medium] run\r\n");
        rtos_delay(MEDIUM_PERIOD_TICKS);
    }
}

void scheduler_demo_init(void)
{
    rtos_status_t status_high;
    rtos_status_t status_medium;

    status_high = rtos_task_create_static(&high_tcb, "high", high_task, NULL, high_stack,
                                          STACK_WORDS, HIGH_PRIORITY);

    status_medium = rtos_task_create_static(&medium_tcb, "medium", medium_task, NULL, medium_stack,
                                            STACK_WORDS, MEDIUM_PRIORITY);

    if ((status_high != RTOS_OK) || (status_medium != RTOS_OK))
    {
        for (;;)
        {
        }
    }
}

void scheduler_demo_start(void)
{
    rtos_start(&high_tcb, board_cpu_clock_hz());
}

int main(void)
{
    board_init();
    rtos_init();

    scheduler_demo_init();
    scheduler_demo_start();

    for (;;)
    {
    }
}
