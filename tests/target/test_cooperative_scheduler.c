/*
 * TEST-ONLY: include header nội bộ kernel để dùng NTOS_ASSERT trực tiếp
 * kiểm tra invariant (local_marker), đúng ngoại lệ cho phép ở
 * NTOS_CODING_CONVENTION.md muc 2.4.
 */
#include <ntos/ntos.h>
#include <board.h>
#include "ntos_assert_internal.h"
#include <stddef.h>
#include <stdint.h>

#define STACK_WORDS ((size_t)64U)
#define TOGGLE_EVERY 50000U

static ntos_tcb_t led0_task_tcb;
static ntos_tcb_t led1_task_tcb;
static _Alignas(8) uint32_t led0_task_stack[STACK_WORDS];
static _Alignas(8) uint32_t led1_task_stack[STACK_WORDS];

/* Debugger doc cac bien nay de kiem chung ca 2 task deu chay va yield deu dan. */
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

        NTOS_ASSERT(local_marker == 0xA0A0A0A0U);

        ntos_yield();
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

        NTOS_ASSERT(local_marker == 0xB1B1B1B1U);

        ntos_yield();
    }
}

int main(void)
{
    ntos_status_t status_led0;
    ntos_status_t status_led1;

    board_init();

    status_led0 = ntos_task_create_static(&led0_task_tcb, "led0", led0_task, NULL,
        led0_task_stack, STACK_WORDS, 0U);
    status_led1 = ntos_task_create_static(&led1_task_tcb, "led1", led1_task, NULL,
        led1_task_stack, STACK_WORDS, 0U);

    if ((status_led0 != NTOS_OK) || (status_led1 != NTOS_OK))
    {
        for (;;)
        {
        }
    }

    ntos_start(&led0_task_tcb);

    for (;;)
    {
    }
}
