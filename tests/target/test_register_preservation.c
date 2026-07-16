/*
 * TEST-ONLY: include header nội bộ kernel để dùng NTOS_ASSERT trực tiếp,
 * đúng ngoại lệ cho phép ở NTOS_CODING_CONVENTION.md muc 2.4.
 */
#include <ntos/ntos.h>
#include "ntos_assert_internal.h"
#include <stddef.h>
#include <stdint.h>

#define STACK_WORDS ((size_t)64U)

#define TASK_A_PATTERN_BASE 0xA0000000U
#define TASK_B_PATTERN_BASE 0xB0000000U

/* Implement trong test_register_preservation_asm.S - xem chu thich o do. */
extern uint32_t test_yield_check_r4_r11(uint32_t base);

static ntos_tcb_t task_a_tcb;
static ntos_tcb_t task_b_tcb;
static _Alignas(8) uint32_t task_a_stack[STACK_WORDS];
static _Alignas(8) uint32_t task_b_stack[STACK_WORDS];

/* Debugger doc cac bien nay de kiem chung ca 2 task deu pass lien tuc. */
static volatile uint32_t g_task_a_pass_count;
static volatile uint32_t g_task_a_fail_count;
static volatile uint32_t g_task_b_pass_count;
static volatile uint32_t g_task_b_fail_count;

static void task_a(void *argument)
{
    (void)argument;

    for (;;)
    {
        if (test_yield_check_r4_r11(TASK_A_PATTERN_BASE) != 0U)
        {
            g_task_a_pass_count++;
        }
        else
        {
            g_task_a_fail_count++;
            NTOS_ASSERT(false); /* dung ngay tai diem loi that de debugger bat duoc */
        }
    }
}

static void task_b(void *argument)
{
    (void)argument;

    for (;;)
    {
        if (test_yield_check_r4_r11(TASK_B_PATTERN_BASE) != 0U)
        {
            g_task_b_pass_count++;
        }
        else
        {
            g_task_b_fail_count++;
            NTOS_ASSERT(false);
        }
    }
}

int main(void)
{
    ntos_status_t status_a;
    ntos_status_t status_b;

    status_a = ntos_task_create_static(&task_a_tcb, "task_a", task_a, NULL,
        task_a_stack, STACK_WORDS, 0U);
    status_b = ntos_task_create_static(&task_b_tcb, "task_b", task_b, NULL,
        task_b_stack, STACK_WORDS, 0U);

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
