/*
 * Integration test cho Chặng 9A - semaphore CORE (chỉ task context, KHÔNG
 * ISR - xem chang09_semaphore_isr.c cho phần ISR give).
 *
 * Kiến trúc: conductor priority cao nhất (7, không bao giờ bị preempt giữa
 * các bước) điều khiển toàn bộ kịch bản. waiter_high(5)/waiter_low(2) chỉ
 * block một lần lúc khởi động rồi để conductor điều khiển nhịp give().
 *
 * Test 1-3 (không cần block, chạy tuần tự trong conductor):
 *   1. init đúng: count/max_count/waiting_tasks đúng giá trị.
 *   2. count > 0: take() thành công, count giảm.
 *   3. count == 0 + NTOS_NO_WAIT: trả NTOS_ERR_TIMEOUT, không block.
 *
 * Test 4-8 (blocking, direct handoff, priority wake):
 *   4-5. waiter_high/waiter_low cùng ntos_sem_take(WAIT_FOREVER) khi count==0
 *        -> cả hai BLOCKED đúng invariant (ready_item.container == NULL,
 *        wait_item.container == &sem.waiting_tasks).
 *   6. give() lần 1 -> wake waiter_high (priority cao hơn) trước, waiter_low
 *      vẫn BLOCKED, count vẫn == 0 (direct handoff - không tăng count).
 *   7. give() lần 2 -> wake waiter_low.
 *   8. Cả hai ntos_sem_take() phải trả NTOS_OK sau khi task chạy lại.
 *
 * Test 9-10 (không waiter):
 *   9. give() khi không có waiter -> count tăng.
 *   10. give() khi count == max_count -> NTOS_ERR_RESOURCE_FULL.
 *
 * TEST-ONLY: include thẳng kernel/internal để đọc ready_lists[]/
 * ntos_list_is_empty() (xem NTOS_CODING_CONVENTION.md mục 2.4).
 */
#include <ntos/ntos.h>
#include <board.h>
#include <uart.h>
#include "ntos_kernel_internal.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define STACK_WORDS ((size_t)64U)

#define CONDUCTOR_PRIORITY ((ntos_priority_t)7U)
#define HIGH_PRIORITY ((ntos_priority_t)5U)
#define LOW_PRIORITY ((ntos_priority_t)2U)

#define SETTLE_TICKS ((ntos_tick_t)5U)
#define HEARTBEAT_TICKS ((ntos_tick_t)2000U)
#define WAITER_ALIVE_DELAY_TICKS ((ntos_tick_t)1000U)

static ntos_tcb_t task_conductor_tcb;
static ntos_tcb_t task_waiter_high_tcb;
static ntos_tcb_t task_waiter_low_tcb;

static _Alignas(8) uint32_t task_conductor_stack[STACK_WORDS];
static _Alignas(8) uint32_t task_waiter_high_stack[STACK_WORDS];
static _Alignas(8) uint32_t task_waiter_low_stack[STACK_WORDS];

static ntos_semaphore_t sem;

static volatile ntos_status_t g_waiter_high_take_status = NTOS_ERR_INTERNAL;
static volatile ntos_status_t g_waiter_low_take_status = NTOS_ERR_INTERNAL;

static void uart_write_pass_fail(const char *name, bool ok)
{
    bsp_uart_write_string("[9A] ");
    bsp_uart_write_string(name);
    bsp_uart_write_string(ok ? ": PASS\r\n" : ": FAIL\r\n");
}

static bool waiter_blocked_correctly(const ntos_tcb_t *tcb)
{
    return (tcb->state == NTOS_TASK_BLOCKED) &&
           (tcb->ready_item.container == NULL) &&
           (tcb->wait_item.container == &sem.waiting_tasks);
}

static void task_waiter_high(void *argument)
{
    (void)argument;

    g_waiter_high_take_status = ntos_sem_take(&sem, NTOS_WAIT_FOREVER);

    for (;;)
    {
        ntos_delay(WAITER_ALIVE_DELAY_TICKS);
    }
}

static void task_waiter_low(void *argument)
{
    (void)argument;

    g_waiter_low_take_status = ntos_sem_take(&sem, NTOS_WAIT_FOREVER);

    for (;;)
    {
        ntos_delay(WAITER_ALIVE_DELAY_TICKS);
    }
}

static void task_conductor(void *argument)
{
    ntos_status_t status;

    (void)argument;

    bsp_uart_write_string("[9A] chang09_semaphore: bat dau\r\n");

    /* Test 1: init - count>0 để Test 2 lấy được ngay. */
    ntos_sem_init(&sem, 1U, 1U);
    uart_write_pass_fail("init_fields_correct",
        (sem.count == 1U) && (sem.max_count == 1U) && ntos_list_is_empty(&sem.waiting_tasks));

    /* Test 2: count>0 -> take thành công, count giảm. */
    status = ntos_sem_take(&sem, NTOS_NO_WAIT);
    uart_write_pass_fail("take_success_decrements_count", (status == NTOS_OK) && (sem.count == 0U));

    /* Test 3: count==0 + NO_WAIT -> TIMEOUT, không block. */
    status = ntos_sem_take(&sem, NTOS_NO_WAIT);
    uart_write_pass_fail("no_wait_returns_timeout_without_blocking",
        (status == NTOS_ERR_TIMEOUT) && (sem.count == 0U));

    /* Test 4-5: nhường CPU để waiter_high/waiter_low tự block (count vẫn 0). */
    ntos_delay(SETTLE_TICKS);

    uart_write_pass_fail("both_waiters_blocked",
        waiter_blocked_correctly(&task_waiter_high_tcb) &&
        waiter_blocked_correctly(&task_waiter_low_tcb) &&
        (sem.waiting_tasks.length == 2U));

    /* Test 6: give() lần 1 -> wake waiter_high trước (priority cao hơn),
     * direct handoff nên count không đổi. */
    status = ntos_sem_give(&sem);
    uart_write_pass_fail("give_wakes_high_priority_waiter_first_direct_handoff",
        (status == NTOS_OK) && (sem.count == 0U) &&
        (task_waiter_high_tcb.state == NTOS_TASK_READY) &&
        (task_waiter_high_tcb.ready_item.container == &ready_lists[HIGH_PRIORITY]) &&
        waiter_blocked_correctly(&task_waiter_low_tcb));

    /* Test 7: give() lần 2 -> wake waiter_low, hết waiter. */
    status = ntos_sem_give(&sem);
    uart_write_pass_fail("give_wakes_low_priority_waiter",
        (status == NTOS_OK) && (sem.count == 0U) &&
        (task_waiter_low_tcb.state == NTOS_TASK_READY) &&
        ntos_list_is_empty(&sem.waiting_tasks));

    /* Test 8: nhường CPU để cả hai waiter thực sự chạy lại và tự ghi status. */
    ntos_delay(SETTLE_TICKS);

    uart_write_pass_fail("both_waiters_resumed_take_returned_ok",
        (g_waiter_high_take_status == NTOS_OK) && (g_waiter_low_take_status == NTOS_OK));

    /* Test 9: không waiter -> give() tăng count. */
    status = ntos_sem_give(&sem);
    uart_write_pass_fail("give_without_waiter_increments_count", (status == NTOS_OK) && (sem.count == 1U));

    /* Test 10: count == max_count -> RESOURCE_FULL. */
    status = ntos_sem_give(&sem);
    uart_write_pass_fail("give_at_max_count_returns_resource_full",
        (status == NTOS_ERR_RESOURCE_FULL) && (sem.count == 1U));

    bsp_uart_write_string("[9A] chang09_semaphore: xong\r\n");

    for (;;)
    {
        ntos_delay(HEARTBEAT_TICKS);
    }
}

int main(void)
{
    ntos_status_t status_conductor;
    ntos_status_t status_high;
    ntos_status_t status_low;

    board_init();
    ntos_init();

    status_conductor = ntos_task_create_static(&task_conductor_tcb, "conductor", task_conductor,
        NULL, task_conductor_stack, STACK_WORDS, CONDUCTOR_PRIORITY);
    status_high = ntos_task_create_static(&task_waiter_high_tcb, "waiter_high", task_waiter_high,
        NULL, task_waiter_high_stack, STACK_WORDS, HIGH_PRIORITY);
    status_low = ntos_task_create_static(&task_waiter_low_tcb, "waiter_low", task_waiter_low,
        NULL, task_waiter_low_stack, STACK_WORDS, LOW_PRIORITY);

    if ((status_conductor != NTOS_OK) || (status_high != NTOS_OK) || (status_low != NTOS_OK))
    {
        for (;;)
        {
        }
    }

    ntos_start(&task_conductor_tcb);

    for (;;)
    {
    }
}
