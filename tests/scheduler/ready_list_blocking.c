/*
 * Ready list - blocking/wake invariants.
 *
 *   task_b   (priority cao) - delay định kỳ rồi wake, tự kiểm invariant.
 *   task_low (priority thấp) - CPU-bound, quan sát task_b từ bên ngoài trong
 *            lúc nó đang BLOCKED (chạy được vì không còn task priority cao
 *            nào khác giữ CPU trong khoảng đó).
 *
 * Kịch bản:
 *   D. Blocking: khi task_b đang BLOCKED (delay), ready_item.container của nó
 *      phải == NULL - task_low kiểm tra điều này từ bên ngoài.
 *   E. Wake: ngay khi task_b tỉnh dậy, ready_item.container phải ==
 *      &ready_lists[task_b.priority].
 *   F. High wake preempt low: đo thời gian từ lúc gọi delay() đến lúc tỉnh,
 *      phải gần đúng số tick yêu cầu (không bị trễ nhiều vì kẹt sau task_low).
 *   H. Invariant: owner của ready_item phải luôn trỏ đúng TCB sở hữu nó.
 *
 * TEST-ONLY: include thẳng kernel/internal để đọc ready_item.container và
 * mảng ready_lists[] (xem NTOS_CODING_CONVENTION.md mục 2.4).
 */
#include <ntos/ntos.h>
#include <board.h>
#include <uart.h>
#include "ntos_kernel_internal.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define STACK_WORDS ((size_t)64U)
#define HIGH_PRIORITY ((ntos_priority_t)3U)
#define LOW_PRIORITY ((ntos_priority_t)1U)
#define TASK_B_DELAY_TICKS 300U
#define WAKE_LATENCY_TOLERANCE_TICKS 3U
#define LOW_CHECK_EVERY 5000U

static ntos_tcb_t task_b_tcb;
static ntos_tcb_t task_low_tcb;
static _Alignas(8) uint32_t task_b_stack[STACK_WORDS];
static _Alignas(8) uint32_t task_low_stack[STACK_WORDS];

static volatile uint32_t g_task_low_run_count;

static void uart_write_pass_fail(const char *name, bool ok)
{
    bsp_uart_write_string("[ready_list] ");
    bsp_uart_write_string(name);
    bsp_uart_write_string(ok ? ": PASS\r\n" : ": FAIL\r\n");
}

/* Priority cao, duy nhất ở tier này trong file - khi nó BLOCKED (delay),
 * priority thấp (task_low) mới có cơ hội chạy để quan sát từ bên ngoài. */
static void task_b(void *argument)
{
    (void)argument;

    for (;;)
    {
        ntos_tick_t before = ntos_tick_get();
        ntos_delay(TASK_B_DELAY_TICKS);
        ntos_tick_t after = ntos_tick_get();
        ntos_tick_t elapsed = after - before;

        /* E: ngay sau khi wake, ready_item phải nằm đúng ready_lists[priority]. */
        bool wake_container_correct =
            (task_b_tcb.ready_item.container == &ready_lists[task_b_tcb.priority]);
        /* H: owner của ready_item phải luôn trỏ đúng TCB sở hữu nó. */
        bool owner_correct = (task_b_tcb.ready_item.owner == &task_b_tcb);
        /* F: wake phải gần đúng hạn. */
        bool woke_promptly = (elapsed >= TASK_B_DELAY_TICKS) &&
                              (elapsed <= (TASK_B_DELAY_TICKS + WAKE_LATENCY_TOLERANCE_TICKS));

        uart_write_pass_fail("wake_container_matches_ready_list", wake_container_correct);
        uart_write_pass_fail("ready_item_owner_correct", owner_correct);
        uart_write_pass_fail("high_wake_preempts_low_promptly", woke_promptly);
        bsp_uart_write_string("\r\n");
    }
}

/* Priority thấp - luôn READY, chạy xen kẽ mỗi khi task_b đang BLOCKED (vì
 * không còn task priority cao nào khác giữ CPU trong khoảng đó). */
static void task_low(void *argument)
{
    (void)argument;

    for (;;)
    {
        g_task_low_run_count++;

        if ((g_task_low_run_count % LOW_CHECK_EVERY) == 0U)
        {
            if (task_b_tcb.state == NTOS_TASK_BLOCKED)
            {
                /* D: BLOCKED task tuyệt đối không còn nằm trong ready list. */
                bool blocked_container_null = (task_b_tcb.ready_item.container == NULL);
                uart_write_pass_fail("blocked_task_container_null", blocked_container_null);
            }
        }
    }
}

int main(void)
{
    ntos_status_t status_b;
    ntos_status_t status_low;

    board_init();
    ntos_init();

    status_b = ntos_task_create_static(&task_b_tcb, "task_b", task_b, NULL,
        task_b_stack, STACK_WORDS, HIGH_PRIORITY);
    status_low = ntos_task_create_static(&task_low_tcb, "task_low", task_low, NULL,
        task_low_stack, STACK_WORDS, LOW_PRIORITY);

    if ((status_b != NTOS_OK) || (status_low != NTOS_OK))
    {
        for (;;)
        {
        }
    }

    ntos_start(&task_b_tcb);

    for (;;)
    {
    }
}
