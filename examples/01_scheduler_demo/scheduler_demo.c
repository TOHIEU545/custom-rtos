#include "scheduler_demo.h"

#include <ntos/ntos.h>
#include <uart.h>

#include <stddef.h>
#include <stdint.h>

#define STACK_WORDS ((size_t)64U)

#define HIGH_PRIORITY ((ntos_priority_t)2U)
#define MEDIUM_PRIORITY ((ntos_priority_t)1U)

#define HIGH_PERIOD_TICKS ((ntos_tick_t)1000U)
#define MEDIUM_PERIOD_TICKS ((ntos_tick_t)2000U)

static ntos_tcb_t high_tcb;
static ntos_tcb_t medium_tcb;

static _Alignas(8) uint32_t high_stack[STACK_WORDS];
static _Alignas(8) uint32_t medium_stack[STACK_WORDS];

static void high_task(void *argument)
{
    (void)argument;

    for (;;)
    {
        bsp_uart_write_string("[high] run\r\n");
        ntos_delay(HIGH_PERIOD_TICKS);
    }
}

static void medium_task(void *argument)
{
    (void)argument;

    for (;;)
    {
        bsp_uart_write_string("[medium] run\r\n");
        ntos_delay(MEDIUM_PERIOD_TICKS);
    }
}

void scheduler_demo_init(void)
{
    ntos_status_t status_high;
    ntos_status_t status_medium;

    status_high = ntos_task_create_static(&high_tcb, "high", high_task, NULL, high_stack,
                                          STACK_WORDS, HIGH_PRIORITY);

    status_medium = ntos_task_create_static(&medium_tcb, "medium", medium_task, NULL, medium_stack,
                                            STACK_WORDS, MEDIUM_PRIORITY);

    if ((status_high != NTOS_OK) || (status_medium != NTOS_OK))
    {
        for (;;)
        {
        }
    }
}

void scheduler_demo_start(void)
{
    ntos_start(&high_tcb);
}