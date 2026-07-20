#include "scheduler_demo.h"

#include <ntos/ntos.h>
#include <uart.h>
#include <stddef.h>
#include <stdint.h>

#define STACK_WORDS ((size_t)64U)

#define HIGH_PRIORITY ((ntos_priority_t)3U)
#define MEDIUM_PRIORITY ((ntos_priority_t)2U)
#define LOGGER_PRIORITY ((ntos_priority_t)1U)

#define HIGH_PERIOD_TICKS ((ntos_tick_t)400U)
#define MEDIUM_PERIOD_TICKS ((ntos_tick_t)150U)
#define LOGGER_PERIOD_TICKS ((ntos_tick_t)2000U)

static ntos_tcb_t high_tcb;
static ntos_tcb_t medium_tcb;
static ntos_tcb_t logger_tcb;

static _Alignas(8) uint32_t high_stack[STACK_WORDS];
static _Alignas(8) uint32_t medium_stack[STACK_WORDS];
static _Alignas(8) uint32_t logger_stack[STACK_WORDS];

/* logger_task là task duy nhất đọc hai counter này để in UART. */
static volatile uint32_t g_high_count;
static volatile uint32_t g_medium_count;

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

/* Priority cao nhất - mỗi lần wake sẽ preempt medium_task ngay lập tức nếu nó
 * đang chạy. Tự delay nên cũng nhường CPU đều đặn, không chiếm CPU vĩnh viễn. */
static void high_task(void *argument)
{
    (void)argument;

    for (;;)
    {
        g_high_count++;
        ntos_delay(HIGH_PERIOD_TICKS);
    }
}

/* Priority trung bình - period ngắn hơn high nên chạy thường xuyên hơn, nhưng
 * vẫn tự delay sau mỗi lần chạy để logger_task (priority thấp nhất) có cơ hội
 * được scheduler chọn. */
static void medium_task(void *argument)
{
    (void)argument;

    for (;;)
    {
        g_medium_count++;
        ntos_delay(MEDIUM_PERIOD_TICKS);
    }
}

/* Priority thấp nhất - task duy nhất ghi UART, đọc counter định kỳ. */
static void logger_task(void *argument)
{
    (void)argument;

    for (;;)
    {
        ntos_delay(LOGGER_PERIOD_TICKS);

        bsp_uart_write_string("[scheduler] high=");
        uart_write_uint32(g_high_count);
        bsp_uart_write_string(" medium=");
        uart_write_uint32(g_medium_count);
        bsp_uart_write_string("\r\n");
    }
}

void scheduler_demo_init(void)
{
    ntos_status_t status_high;
    ntos_status_t status_medium;
    ntos_status_t status_logger;

    status_high = ntos_task_create_static(&high_tcb, "high", high_task, NULL,
        high_stack, STACK_WORDS, HIGH_PRIORITY);
    status_medium = ntos_task_create_static(&medium_tcb, "medium", medium_task, NULL,
        medium_stack, STACK_WORDS, MEDIUM_PRIORITY);
    status_logger = ntos_task_create_static(&logger_tcb, "logger", logger_task, NULL,
        logger_stack, STACK_WORDS, LOGGER_PRIORITY);

    if ((status_high != NTOS_OK) || (status_medium != NTOS_OK) || (status_logger != NTOS_OK))
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
