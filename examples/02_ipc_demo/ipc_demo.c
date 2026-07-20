#include "ipc_demo.h"

#include <ntos/ntos.h>
#include <board.h>
#include <exti.h>
#include <uart.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define STACK_WORDS ((size_t)64U)

#define CONTROL_PRIORITY ((ntos_priority_t)3U)
#define EVENT_PRIORITY ((ntos_priority_t)2U)
#define LOGGER_PRIORITY ((ntos_priority_t)1U)

#define EVENT_QUEUE_CAPACITY ((size_t)4U)
#define LOGGER_PERIOD_TICKS ((ntos_tick_t)2000U)

static ntos_tcb_t control_tcb;
static ntos_tcb_t event_tcb;
static ntos_tcb_t logger_tcb;

static _Alignas(8) uint32_t control_stack[STACK_WORDS];
static _Alignas(8) uint32_t event_stack[STACK_WORDS];
static _Alignas(8) uint32_t logger_stack[STACK_WORDS];

static ntos_semaphore_t control_sem;

static ntos_queue_t event_queue;
static uint32_t event_queue_storage[EVENT_QUEUE_CAPACITY];

/* logger_task là task duy nhất đọc hai counter này để in UART. */
static volatile uint32_t g_control_count;
static volatile uint32_t g_event_count;

/* Sequence number cho item gửi qua queue - chỉ ISR ghi, không liên quan tới
 * g_event_count (số item event_task đã thực sự nhận, logger đọc để in). */
static volatile uint32_t g_event_sequence;

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

/* KEY0 -> ntos_sem_give_from_isr() -> đây thức dậy. Không ghi UART. */
static void control_task(void *argument)
{
    (void)argument;

    for (;;)
    {
        (void)ntos_sem_take(&control_sem, NTOS_WAIT_FOREVER);
        g_control_count++;
        board_led_toggle(BOARD_LED_D0);
    }
}

/* KEY1 -> ntos_queue_send_from_isr() -> đây thức dậy. Không ghi UART. */
static void event_task(void *argument)
{
    uint32_t event;
    (void)argument;

    for (;;)
    {
        (void)ntos_queue_receive(&event_queue, &event, NTOS_WAIT_FOREVER);
        g_event_count++;
        board_led_toggle(BOARD_LED_D1);
    }
}

/* Priority thấp nhất - task duy nhất ghi UART. Bật EXTI ở đây (không phải
 * main()) để control_task/event_task chắc chắn đã BLOCKED chờ trước khi
 * ngắt nút bấm được enable. */
static void logger_task(void *argument)
{
    (void)argument;

    bsp_exti_config(BOARD_KEY0_PORT, BOARD_KEY0_PIN, BSP_EXTI_TRIGGER_FALLING);
    bsp_exti_config(BOARD_KEY1_PORT, BOARD_KEY1_PIN, BSP_EXTI_TRIGGER_FALLING);

    for (;;)
    {
        ntos_delay(LOGGER_PERIOD_TICKS);

        bsp_uart_write_string("[ipc] control=");
        uart_write_uint32(g_control_count);
        bsp_uart_write_string(" events=");
        uart_write_uint32(g_event_count);
        bsp_uart_write_string("\r\n");
    }
}

/* ISR ngắn: chỉ clear pending + give. Không UART, không xử lý logic. */
void EXTI1_IRQHandler(void)
{
    bool higher_priority_task_woken = false;

    bsp_exti_clear_pending(BOARD_KEY0_PIN);
    (void)ntos_sem_give_from_isr(&control_sem, &higher_priority_task_woken);
}

/* EXTI15_10 dùng chung cho line 10-15 - phải tự kiểm đúng line của KEY1
 * trước khi xử lý. ISR ngắn: chỉ clear pending + send. */
void EXTI15_10_IRQHandler(void)
{
    if (bsp_exti_pending(BOARD_KEY1_PIN))
    {
        bool higher_priority_task_woken = false;
        uint32_t event;

        bsp_exti_clear_pending(BOARD_KEY1_PIN);

        event = ++g_event_sequence;
        (void)ntos_queue_send_from_isr(&event_queue, &event, &higher_priority_task_woken);
    }
}

void ipc_demo_init(void)
{
    ntos_status_t status_queue;
    ntos_status_t status_control;
    ntos_status_t status_event;
    ntos_status_t status_logger;

    ntos_sem_init(&control_sem, 0U, 1U);
    status_queue = ntos_queue_init_static(&event_queue, event_queue_storage, sizeof(uint32_t),
        EVENT_QUEUE_CAPACITY);

    status_control = ntos_task_create_static(&control_tcb, "control", control_task, NULL,
        control_stack, STACK_WORDS, CONTROL_PRIORITY);
    status_event = ntos_task_create_static(&event_tcb, "event", event_task, NULL,
        event_stack, STACK_WORDS, EVENT_PRIORITY);
    status_logger = ntos_task_create_static(&logger_tcb, "logger", logger_task, NULL,
        logger_stack, STACK_WORDS, LOGGER_PRIORITY);

    if ((status_queue != NTOS_OK) || (status_control != NTOS_OK) ||
        (status_event != NTOS_OK) || (status_logger != NTOS_OK))
    {
        for (;;)
        {
        }
    }
}

void ipc_demo_start(void)
{
    ntos_start(&control_tcb);
}
