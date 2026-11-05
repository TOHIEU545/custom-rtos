#include <rtos.h>

#include <bsp.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define STACK_WORDS ((size_t)64U)

#define CONTROL_PRIORITY ((rtos_priority_t)6U)
#define PARSER_PRIORITY ((rtos_priority_t)4U)
#define LOGGER_PRIORITY ((rtos_priority_t)2U)

#define EVENT_QUEUE_CAPACITY ((size_t)4U)
#define LOGGER_PERIOD_TICKS ((rtos_tick_t)2000U)

static rtos_tcb_t control_tcb;
static rtos_tcb_t parser_tcb;
static rtos_tcb_t logger_tcb;

static _Alignas(8) uint32_t control_stack[STACK_WORDS];
static _Alignas(8) uint32_t parser_stack[STACK_WORDS];
static _Alignas(8) uint32_t logger_stack[STACK_WORDS];

static rtos_semaphore_t control_sem;

static rtos_queue_t event_queue;
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
        rtos_sem_take(&control_sem, RTOS_WAIT_FOREVER);

        g_control_count++;
        board_led_toggle(BOARD_LED_D0);

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
        rtos_queue_receive(&event_queue, &event, RTOS_WAIT_FOREVER);

        g_parser_count++;
        board_led_toggle(BOARD_LED_D1);

        bsp_uart_write_string("parser event: ");
        uart_write_uint32(event);
        bsp_uart_write_string("\r\n");
    }
}

static void logger_task(void *argument)
{
    (void)argument;

    /*
     * Higher-priority tasks have already blocked before logger runs,
     * so the waiters are ready before interrupts are enabled.
     */
    bsp_exti_config(BOARD_KEY0_PORT, BOARD_KEY0_PIN, BSP_EXTI_TRIGGER_FALLING);

    bsp_exti_config(BOARD_KEY1_PORT, BOARD_KEY1_PIN, BSP_EXTI_TRIGGER_FALLING);

    for (;;)
    {
        rtos_delay(LOGGER_PERIOD_TICKS);

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
    uint32_t event;

    bsp_exti_clear_pending(BOARD_KEY0_PIN);

    event = ++g_event_counter;

    (void)rtos_queue_send_from_isr(&event_queue, &event, &higher_priority_task_woken);
}

void EXTI15_10_IRQHandler(void)
{
    if (bsp_exti_pending(BOARD_KEY1_PIN))
    {
        bool higher_priority_task_woken = false;

        bsp_exti_clear_pending(BOARD_KEY1_PIN);

        (void)rtos_sem_give_from_isr(&control_sem, &higher_priority_task_woken);
    }
}

int main(void)
{
    board_init();
    rtos_init();

    rtos_sem_init(&control_sem, 0U, 1U);

    if (rtos_queue_init_static(&event_queue, event_queue_storage, sizeof(uint32_t),
                               EVENT_QUEUE_CAPACITY) != RTOS_OK)
    {
        for (;;)
        {
        }
    }

    if (rtos_task_create_static(&control_tcb, "control", control_task, NULL, control_stack,
                                STACK_WORDS, CONTROL_PRIORITY) != RTOS_OK)
    {
        for (;;)
        {
        }
    }

    if (rtos_task_create_static(&parser_tcb, "parser", parser_task, NULL, parser_stack, STACK_WORDS,
                                PARSER_PRIORITY) != RTOS_OK)
    {
        for (;;)
        {
        }
    }

    if (rtos_task_create_static(&logger_tcb, "logger", logger_task, NULL, logger_stack, STACK_WORDS,
                                LOGGER_PRIORITY) != RTOS_OK)
    {
        for (;;)
        {
        }
    }

    rtos_start(&control_tcb, board_cpu_clock_hz());

    for (;;)
    {
    }
}