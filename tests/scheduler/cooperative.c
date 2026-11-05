/*
 * TEST-ONLY: include header nội bộ kernel để dùng RTOS_ASSERT trực tiếp
 * kiểm tra invariant (local_marker), đúng ngoại lệ cho phép ở
 * RTOS_CODING_CONVENTION.md mục 2.4.
 */
#include <rtos.h>
#include <bsp.h>
#include "kernel_internal.h"
#include <stddef.h>
#include <stdint.h>

#define STACK_WORDS ((size_t)64U)
#define TOGGLE_EVERY 50000U

static rtos_tcb_t led0_task_tcb;
static rtos_tcb_t led1_task_tcb;
static _Alignas(8) uint32_t led0_task_stack[STACK_WORDS];
static _Alignas(8) uint32_t led1_task_stack[STACK_WORDS];

/* Debugger đọc các biến này để kiểm chứng cả 2 task đều chạy và yield đều đặn. */
static volatile uint32_t led0_counter;
static volatile uint32_t led1_counter;

static void led0_task(void *argument)
{
    uint32_t local_marker = 0xA0A0A0A0U;
    (void)argument;

    for (;;)
    {
        led0_counter++;
        if ((led0_counter % TOGGLE_EVERY) == 0U)
        {
            board_led_toggle(BOARD_LED_D0);
        }

        RTOS_ASSERT(local_marker == 0xA0A0A0A0U);

        rtos_yield();
    }
}

static void led1_task(void *argument)
{
    uint32_t local_marker = 0xB1B1B1B1U;
    (void)argument;

    for (;;)
    {
        led1_counter++;
        if ((led1_counter % TOGGLE_EVERY) == 0U)
        {
            board_led_toggle(BOARD_LED_D1);
        }

        RTOS_ASSERT(local_marker == 0xB1B1B1B1U);

        rtos_yield();
    }
}

int main(void)
{
    rtos_status_t status_led0;
    rtos_status_t status_led1;

    board_init();
    /* rtos_init() phải gọi trước khi tạo task bất kỳ nào - nó init
     * ready_lists[] mà rtos_task_create_static() sẽ đưa task mới vào ngay. */
    rtos_init();

    status_led0 = rtos_task_create_static(&led0_task_tcb, "led0", led0_task, NULL,
        led0_task_stack, STACK_WORDS, 0U);
    status_led1 = rtos_task_create_static(&led1_task_tcb, "led1", led1_task, NULL,
        led1_task_stack, STACK_WORDS, 0U);

    if ((status_led0 != RTOS_OK) || (status_led1 != RTOS_OK))
    {
        for (;;)
        {
        }
    }

    rtos_start(&led0_task_tcb, board_cpu_clock_hz());

    for (;;)
    {
    }
}
