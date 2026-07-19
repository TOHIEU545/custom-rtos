/*
 * Task blocks on a semaphore; pressing the button gives it from an EXTI ISR.
 *
 * The ISR only clears the pending bit and gives the semaphore - it does not
 * print, so it stays as short as possible. The task itself logs the wake.
 */
#include <ntos/ntos.h>
#include <board.h>
#include <exti.h>
#include <uart.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define STACK_WORDS ((size_t)64U)
#define PRIORITY ((ntos_priority_t)1U)

static ntos_tcb_t task_tcb;
static _Alignas(8) uint32_t task_stack[STACK_WORDS];

static ntos_semaphore_t button_sem;

static void task_wait_for_button(void *argument)
{
    (void)argument;

    for (;;)
    {
        bsp_uart_write_string("[task] wait\r\n");
        (void)ntos_sem_take(&button_sem, NTOS_WAIT_FOREVER);

        bsp_uart_write_string("[button] irq\r\n");
        bsp_uart_write_string("[task] acquired\r\n");
    }
}

void EXTI1_IRQHandler(void)
{
    bool higher_priority_task_woken = false;

    bsp_exti_clear_pending(BOARD_KEY0_PIN);
    (void)ntos_sem_give_from_isr(&button_sem, &higher_priority_task_woken);
}

int main(void)
{
    ntos_status_t status;

    board_init();
    ntos_init();

    ntos_sem_init(&button_sem, 0U, 1U);
    bsp_exti_config(BOARD_KEY0_PORT, BOARD_KEY0_PIN, BSP_EXTI_TRIGGER_FALLING);

    status = ntos_task_create_static(&task_tcb, "button_task", task_wait_for_button, NULL,
        task_stack, STACK_WORDS, PRIORITY);

    if (status != NTOS_OK)
    {
        for (;;)
        {
        }
    }

    ntos_start(&task_tcb);

    for (;;)
    {
    }
}
