/*
 * Ready list - priority selection + same-priority round-robin.
 *
 *   task_a   (priority cao) - CPU-bound, không bao giờ delay.
 *   task_b   (priority cao, bằng task_a) - delay định kỳ rồi wake, tự báo
 *            cáo PASS/FAIL sau mỗi lần wake.
 *   task_low (priority thấp) - CPU-bound, không bao giờ delay.
 *
 * Kịch bản:
 *   A. Highest priority: priority thấp không bao giờ được chọn khi vẫn còn
 *      task priority cao READY - task_a không bao giờ delay nên ready list
 *      priority cao luôn có ít nhất 1 phần tử, task_low bị starve vĩnh viễn.
 *   B. Same priority round-robin: task_a và task_b (cùng priority) thay phiên
 *      nhau mỗi khi cả 2 cùng READY - kiểm qua g_task_a_run_count vẫn tăng dù
 *      task_b vừa mới resume.
 */
#include <ntos/ntos.h>
#include <board.h>
#include <uart.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define STACK_WORDS ((size_t)64U)
#define HIGH_PRIORITY ((ntos_priority_t)3U)
#define LOW_PRIORITY ((ntos_priority_t)1U)
#define TASK_B_DELAY_TICKS 300U

static ntos_tcb_t task_a_tcb;
static ntos_tcb_t task_b_tcb;
static ntos_tcb_t task_low_tcb;
static _Alignas(8) uint32_t task_a_stack[STACK_WORDS];
static _Alignas(8) uint32_t task_b_stack[STACK_WORDS];
static _Alignas(8) uint32_t task_low_stack[STACK_WORDS];

/* Debugger/UART đọc các biến này để kiểm chứng từng kịch bản. */
static volatile uint32_t g_task_a_run_count;
static volatile uint32_t g_task_low_run_count;

static void uart_write_pass_fail(const char *name, bool ok)
{
    bsp_uart_write_string("[ready_list] ");
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

/* Priority cao, delay định kỳ - mỗi lần wake tự kiểm A/B rồi báo cáo. */
static void task_b(void *argument)
{
    (void)argument;

    for (;;)
    {
        ntos_delay(TASK_B_DELAY_TICKS);

        /* B: task_a phải vẫn được chạy trong lúc task_b đang BLOCKED. */
        uart_write_pass_fail("same_priority_round_robin_a_ran", g_task_a_run_count > 0U);

        /* A: task_low priority thấp hơn task_a/task_b - không bao giờ được
         * chọn khi task_a vẫn còn READY (task_a không bao giờ delay). */
        uart_write_pass_fail("low_priority_never_ran_while_high_ready", g_task_low_run_count == 0U);
        bsp_uart_write_string("\r\n");
    }
}

/* Priority thấp - dùng để chứng minh scenario A: không bao giờ được scheduler
 * chọn chạy, nên g_task_low_run_count phải đứng yên ở 0 suốt vòng đời test. */
static void task_low(void *argument)
{
    (void)argument;

    for (;;)
    {
        g_task_low_run_count++;
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
