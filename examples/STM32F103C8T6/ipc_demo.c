#include <rtos.h>
#include <bsp.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define STACK_WORDS ((size_t)64U)

#define CONTROL_PRIORITY ((rtos_priority_t)2U)
#define EVENT_PRIORITY ((rtos_priority_t)1U)

#define EVENT_QUEUE_CAPACITY ((size_t)4U)

static rtos_tcb_t control_tcb;
static rtos_tcb_t event_tcb;

static _Alignas(8) uint32_t control_stack[STACK_WORDS];
static _Alignas(8) uint32_t event_stack[STACK_WORDS];

static rtos_semaphore_t control_sem;

static rtos_queue_t event_queue;
static uint32_t event_queue_storage[EVENT_QUEUE_CAPACITY];

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
static void control_task(void *argument)
{
    (void)argument;

    for (;;)
    {
        (void)rtos_sem_take(&control_sem, RTOS_WAIT_FOREVER);

        board_led_toggle(BOARD_LED_D0);
        bsp_uart_write_string("[ipc] semaphore received\r\n");
    }
}

static void event_task(void *argument)
{
    uint32_t event;

    (void)argument;

    for (;;)
    {
        (void)rtos_queue_receive(&event_queue, &event, RTOS_WAIT_FOREVER);

        board_led_toggle(BOARD_LED_D1);

        bsp_uart_write_string("[ipc] queue event=");
        uart_write_uint32(event);
        bsp_uart_write_string("\r\n");
    }
}

void EXTI1_IRQHandler(void)
{
    bool higher_priority_task_woken = false;

    bsp_exti_clear_pending(BOARD_KEY0_PIN);

    (void)rtos_sem_give_from_isr(&control_sem, &higher_priority_task_woken);
}

void EXTI15_10_IRQHandler(void)
{
    if (bsp_exti_pending(BOARD_KEY1_PIN))
    {
        bool higher_priority_task_woken = false;
        uint32_t event;

        bsp_exti_clear_pending(BOARD_KEY1_PIN);

        event = ++g_event_sequence;

        (void)rtos_queue_send_from_isr(&event_queue, &event, &higher_priority_task_woken);
    }
}

void ipc_demo_init(void)
{
    rtos_status_t status_queue;
    rtos_status_t status_control;
    rtos_status_t status_event;

    rtos_sem_init(&control_sem, 0U, 1U);

    status_queue = rtos_queue_init_static(&event_queue, event_queue_storage, sizeof(uint32_t),
                                          EVENT_QUEUE_CAPACITY);

    status_control = rtos_task_create_static(&control_tcb, "control", control_task, NULL,
                                             control_stack, STACK_WORDS, CONTROL_PRIORITY);

    status_event = rtos_task_create_static(&event_tcb, "event", event_task, NULL, event_stack,
                                           STACK_WORDS, EVENT_PRIORITY);

    if ((status_queue != RTOS_OK) || (status_control != RTOS_OK) || (status_event != RTOS_OK))
    {
        for (;;)
        {
        }
    }

    bsp_exti_config(BOARD_KEY0_PORT, BOARD_KEY0_PIN, BSP_EXTI_TRIGGER_FALLING);

    bsp_exti_config(BOARD_KEY1_PORT, BOARD_KEY1_PIN, BSP_EXTI_TRIGGER_FALLING);
}

void ipc_demo_start(void)
{
    rtos_start(&control_tcb, board_cpu_clock_hz());
}

int main(void)
{
    board_init();
    rtos_init();

    ipc_demo_init();
    ipc_demo_start();

    for (;;)
    {
    }
}
