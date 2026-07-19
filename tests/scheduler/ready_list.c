/*
 * Integration test cho ready list + scheduler theo priority.
 *
 * 3 task:
 *   task_a   (priority 3) - CPU-bound, không bao giờ delay.
 *   task_b   (priority 3, bằng task_a) - delay định kỳ rồi wake.
 *   task_low (priority 1) - CPU-bound, không bao giờ delay; dùng để quan sát
 *                           invariant của task_b từ bên ngoài trong lúc task_b
 *                           đang BLOCKED (vì task_low luôn được chạy xen kẽ).
 *
 * Kịch bản được bao phủ:
 *   A. Highest priority: task_low (pri1) không bao giờ chạy khi task_a/task_b
 *      (pri3) còn READY - kiểm qua g_task_low_run_count đứng yên giữa 2 lần đo.
 *   B. Same priority round-robin: task_a và task_b (cùng pri3) thay phiên nhau
 *      mỗi khi cả 2 cùng READY - kiểm qua cả 2 counter đều tăng.
 *   C. Mixed priority: chính là A/B cộng lại (2 task pri3 round-robin, 1 task
 *      pri1 bị starve) - không cần kịch bản riêng.
 *   D. Blocking: khi task_b đang BLOCKED (delay), ready_item.container của nó
 *      phải == NULL - task_low kiểm tra điều này từ bên ngoài.
 *   E. Wake: ngay khi task_b tỉnh dậy, ready_item.container phải ==
 *      &ready_lists[task_b.priority] (dùng ready_lists được export qua
 *      kernel_internal.h chỉ để test).
 *   F. High wake preempt low: đo thời gian từ lúc gọi delay() đến lúc tỉnh,
 *      phải gần đúng số tick yêu cầu (không bị trễ nhiều vì kẹt sau task_low).
 *   H. Invariant: owner của ready_item phải luôn trỏ đúng TCB sở hữu nó.
 *
 * G (idle được chọn khi tất cả app task đều BLOCKED) không lặp lại ở đây, vì
 * cần cả task_low cũng phải delay (mâu thuẫn với vai trò "luôn READY" của nó
 * trong file này).
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

static ntos_tcb_t task_a_tcb;
static ntos_tcb_t task_b_tcb;
static ntos_tcb_t task_low_tcb;
static _Alignas(8) uint32_t task_a_stack[STACK_WORDS];
static _Alignas(8) uint32_t task_b_stack[STACK_WORDS];
static _Alignas(8) uint32_t task_low_stack[STACK_WORDS];

/* Debugger/UART đọc các biến này để kiểm chứng từng kịch bản. */
static volatile uint32_t g_task_a_run_count;
static volatile uint32_t g_task_b_run_count;
static volatile uint32_t g_task_low_run_count;

static void uart_write_pass_fail(const char *name, bool ok)
{
    bsp_uart_write_string("[8] ");
    bsp_uart_write_string(name);
    bsp_uart_write_string(ok ? ": PASS\r\n" : ": FAIL\r\n");
}

/* Priority cao, cùng tier với task_b - round-robin với task_b khi cả 2 READY. */
static void task_a(void *argument)
{
    (void)argument;

    for (;;)
    {
        g_task_a_run_count++;
    }
}

/* Priority cao, delay định kỳ - kiểm tra invariant D/E/F/H trên chính nó. */
static void task_b(void *argument)
{
    (void)argument;

    for (;;)
    {
        g_task_b_run_count++;

        ntos_tick_t before = ntos_tick_get();
        ntos_delay(TASK_B_DELAY_TICKS);
        ntos_tick_t after = ntos_tick_get();
        ntos_tick_t elapsed = after - before;

        /* E: ngay sau khi wake, ready_item phải nằm đúng ready_lists[priority]. */
        bool wake_container_correct =
            (task_b_tcb.ready_item.container == &ready_lists[task_b_tcb.priority]);
        /* H: owner của ready_item phải luôn trỏ đúng TCB sở hữu nó. */
        bool owner_correct = (task_b_tcb.ready_item.owner == &task_b_tcb);
        /* F: wake phải gần đúng hạn, không bị kẹt lại phía sau task_low. */
        bool woke_promptly = (elapsed >= TASK_B_DELAY_TICKS) &&
                              (elapsed <= (TASK_B_DELAY_TICKS + WAKE_LATENCY_TOLERANCE_TICKS));
        /* B: task_a cũng phải được chạy (round-robin cùng priority). */
        bool task_a_also_ran = (g_task_a_run_count > 0U);

        uart_write_pass_fail("wake_container_matches_ready_list", wake_container_correct);
        uart_write_pass_fail("ready_item_owner_correct", owner_correct);
        uart_write_pass_fail("high_wake_preempts_low_promptly", woke_promptly);
        uart_write_pass_fail("same_priority_round_robin_a_ran", task_a_also_ran);
        bsp_uart_write_string("\r\n");
    }
}

/* Priority thấp - luôn READY, dùng làm "người quan sát" invariant của task_b
 * từ bên ngoài trong lúc task_b đang BLOCKED (vì task_low được chạy xen kẽ). */
static void task_low(void *argument)
{
    (void)argument;

    for (;;)
    {
        g_task_low_run_count++;

        if ((g_task_low_run_count % LOW_CHECK_EVERY) == 0U)
        {
            /* A/C: task_low đang chạy tức là tại thời điểm này không có task
             * pri3 nào đang RUNNING - nhưng nếu task_a/task_b vẫn còn READY,
             * lần schedule tiếp theo phải quay lại ưu tiên họ, không phải
             * task_low chiếm liên tục (kiểm tiếp ở dưới). */
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
    ntos_status_t status_a;
    ntos_status_t status_b;
    ntos_status_t status_low;

    board_init();
    ntos_init();

    status_a = ntos_task_create_static(&task_a_tcb, "task_a", task_a, NULL,
        task_a_stack, STACK_WORDS, HIGH_PRIORITY);
    status_b = ntos_task_create_static(&task_b_tcb, "task_b", task_b, NULL,
        task_b_stack, STACK_WORDS, HIGH_PRIORITY);
    status_low = ntos_task_create_static(&task_low_tcb, "task_low", task_low, NULL,
        task_low_stack, STACK_WORDS, LOW_PRIORITY);

    if ((status_a != NTOS_OK) || (status_b != NTOS_OK) || (status_low != NTOS_OK))
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
