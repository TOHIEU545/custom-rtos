/*
 * Two CPU-bound tasks, same priority, neither ever calls ntos_yield() or
 * ntos_delay(). SysTick alone forces the context switch between them - if
 * preemption were broken, only [A] would ever print.
 */
#include <ntos/ntos.h>
#include <board.h>
#include <uart.h>
#include <stddef.h>
#include <stdint.h>

#define STACK_WORDS ((size_t)64U)
#define PRIORITY ((ntos_priority_t)1U)
#define PRINT_EVERY 300000U

static ntos_tcb_t task_a_tcb;
static ntos_tcb_t task_b_tcb;
static _Alignas(8) uint32_t task_a_stack[STACK_WORDS];
static _Alignas(8) uint32_t task_b_stack[STACK_WORDS];

static void task_a(void *argument)
{
    uint32_t count = 0U;
    (void)argument;

    for (;;)
    {
        if ((++count % PRINT_EVERY) == 0U)
        {
            bsp_uart_write_string("[A] tick\r\n");
        }
    }
}

static void task_b(void *argument)
{
    uint32_t count = 0U;
    (void)argument;

    for (;;)
    {
        if ((++count % PRINT_EVERY) == 0U)
        {
            bsp_uart_write_string("[B] tick\r\n");
        }
    }
}

int main(void)
{
    ntos_status_t status_a;
    ntos_status_t status_b;

    board_init();
    ntos_init();

    status_a = ntos_task_create_static(&task_a_tcb, "task_a", task_a, NULL,
        task_a_stack, STACK_WORDS, PRIORITY);
    status_b = ntos_task_create_static(&task_b_tcb, "task_b", task_b, NULL,
        task_b_stack, STACK_WORDS, PRIORITY);

    if ((status_a != NTOS_OK) || (status_b != NTOS_OK))
    {
        for (;;)
        {
        }
    }

    ntos_start(&task_a_tcb);

    for (;;)
    {
    }
}
