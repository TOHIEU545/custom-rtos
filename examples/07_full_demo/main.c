/*
 * Short integration demo: semaphore + queue + a periodic status logger,
 * three tasks total.
 *
 *   control - blocks on a semaphore given by KEY0's ISR.
 *   parser  - blocks on a queue fed by KEY1's ISR.
 *   logger  - prints a combined status line every couple seconds.
 */
#include <ntos/ntos.h>
#include <board.h>
#include <exti.h>
#include <uart.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define STACK_WORDS ((size_t)64U)

#define CONTROL_PRIORITY ((ntos_priority_t)3U)
#define PARSER_PRIORITY ((ntos_priority_t)2U)
#define LOGGER_PRIORITY ((ntos_priority_t)1U)

#define EVENT_QUEUE_CAPACITY ((size_t)4U)
#define LOGGER_PERIOD_TICKS ((ntos_tick_t)2000U)

static ntos_tcb_t control_tcb;
static ntos_tcb_t parser_tcb;
static ntos_tcb_t logger_tcb;

static _Alignas(8) uint32_t control_stack[STACK_WORDS];
static _Alignas(8) uint32_t parser_stack[STACK_WORDS];
static _Alignas(8) uint32_t logger_stack[STACK_WORDS];

static ntos_semaphore_t control_sem;

static ntos_queue_t event_queue;
static uint32_t event_queue_storage[EVENT_QUEUE_CAPACITY];

static volatile uint32_t g_control_count;
static volatile uint32_t g_parser_count;
static volatile uint32_t g_event_counter;

static void uart_write_uint32(uint32_t value)
{
    char digits[10];
    size_t count = 0U;

    if (value == 0U)
    {
        bsp_uart_write_byte('0');
        return;
    }

    while (value > 0U)
    {
        digits[count++] = (char)('0' + (value % 10U));
        value /= 10U;
    }

    while (count > 0U)
    {
        bsp_uart_write_byte((uint8_t)digits[--count]);
    }
}

static void control_task(void *argument)
{
    (void)argument;

    for (;;)
    {
        (void)ntos_sem_take(&control_sem, NTOS_WAIT_FOREVER);
        g_control_count++;

        bsp_uart_write_string("control wake: ");
        uart_write_uint32(g_control_count);
        bsp_uart_write_string("\r\n");
    }
}

static void parser_task(void *argument)
{
    uint32_t event;
    (void)argument;

    for (;;)
    {
        (void)ntos_queue_receive(&event_queue, &event, NTOS_WAIT_FOREVER);
        g_parser_count++;

        bsp_uart_write_string("parser event: ");
        uart_write_uint32(event);
        bsp_uart_write_string("\r\n");
    }
}

static void logger_task(void *argument)
{
    (void)argument;

    /* Higher-priority tasks already blocked before logger runs, so both
     * waiters are ready before the buttons' interrupts are enabled here. */
    bsp_exti_config(BOARD_KEY0_PORT, BOARD_KEY0_PIN, BSP_EXTI_TRIGGER_FALLING);
    bsp_exti_config(BOARD_KEY1_PORT, BOARD_KEY1_PIN, BSP_EXTI_TRIGGER_FALLING);

    for (;;)
    {
        ntos_delay(LOGGER_PERIOD_TICKS);

        bsp_uart_write_string("status: control=");
        uart_write_uint32(g_control_count);
        bsp_uart_write_string(" parser=");
        uart_write_uint32(g_parser_count);
        bsp_uart_write_string("\r\n");
    }
}

void EXTI1_IRQHandler(void)
{
    bool higher_priority_task_woken = false;

    bsp_exti_clear_pending(BOARD_KEY0_PIN);
    (void)ntos_sem_give_from_isr(&control_sem, &higher_priority_task_woken);
}

void EXTI15_10_IRQHandler(void)
{
    if (bsp_exti_pending(BOARD_KEY1_PIN))
    {
        bool higher_priority_task_woken = false;
        uint32_t event;

        bsp_exti_clear_pending(BOARD_KEY1_PIN);

        event = ++g_event_counter;
        (void)ntos_queue_send_from_isr(&event_queue, &event, &higher_priority_task_woken);
    }
}

int main(void)
{
    ntos_status_t status_queue;
    ntos_status_t status_control;
    ntos_status_t status_parser;
    ntos_status_t status_logger;

    board_init();
    ntos_init();

    ntos_sem_init(&control_sem, 0U, 1U);
    status_queue = ntos_queue_init_static(&event_queue, event_queue_storage, sizeof(uint32_t),
        EVENT_QUEUE_CAPACITY);

    if (status_queue != NTOS_OK)
    {
        for (;;)
        {
        }
    }

    status_control = ntos_task_create_static(&control_tcb, "control", control_task, NULL,
        control_stack, STACK_WORDS, CONTROL_PRIORITY);
    status_parser = ntos_task_create_static(&parser_tcb, "parser", parser_task, NULL,
        parser_stack, STACK_WORDS, PARSER_PRIORITY);
    status_logger = ntos_task_create_static(&logger_tcb, "logger", logger_task, NULL,
        logger_stack, STACK_WORDS, LOGGER_PRIORITY);

    if ((status_control != NTOS_OK) || (status_parser != NTOS_OK) || (status_logger != NTOS_OK))
    {
        for (;;)
        {
        }
    }

    ntos_start(&control_tcb);

    for (;;)
    {
    }
}
