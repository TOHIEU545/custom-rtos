/*
 * Semaphore blocking contract: two waiters, priority-based wake-up, direct
 * handoff.
 *
 * The conductor task has the highest priority so state checks happen in a
 * deterministic order; waiter tasks run only when the conductor explicitly
 * blocks.
 *
 * TEST-ONLY: include thẳng kernel/src/kernel_internal.h để đọc ready_lists[] (xem
 * RTOS_CODING_CONVENTION.md mục 2.4).
 */
#include <rtos.h>
#include <bsp.h>
#include "kernel_internal.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define STACK_WORDS ((size_t)64U)

#define CONDUCTOR_PRIORITY ((rtos_priority_t)7U)
#define HIGH_PRIORITY ((rtos_priority_t)5U)
#define LOW_PRIORITY ((rtos_priority_t)2U)

#define SETTLE_TICKS ((rtos_tick_t)5U)
#define HEARTBEAT_TICKS ((rtos_tick_t)2000U)
#define WAITER_ALIVE_DELAY_TICKS ((rtos_tick_t)1000U)

static rtos_tcb_t task_conductor_tcb;
static rtos_tcb_t task_waiter_high_tcb;
static rtos_tcb_t task_waiter_low_tcb;

static _Alignas(8) uint32_t task_conductor_stack[STACK_WORDS];
static _Alignas(8) uint32_t task_waiter_high_stack[STACK_WORDS];
static _Alignas(8) uint32_t task_waiter_low_stack[STACK_WORDS];

static rtos_semaphore_t sem;

static volatile rtos_status_t g_waiter_high_take_status = RTOS_ERR_INTERNAL;
static volatile rtos_status_t g_waiter_low_take_status = RTOS_ERR_INTERNAL;

static void uart_write_pass_fail(const char *name, bool ok)
{
    bsp_uart_write_string("[semaphore] ");
    bsp_uart_write_string(name);
    bsp_uart_write_string(ok ? ": PASS\r\n" : ": FAIL\r\n");
}

/* Kiểm cả state lẫn hai list-membership, không chỉ state: một bug chỉ cập
 * nhật state == BLOCKED mà quên gỡ ready_item hoặc quên gắn wait_item vẫn có
 * thể khiến task trông "đã block" trong khi thật ra vẫn còn trong ready list
 * (bị scheduler chọn nhầm) hoặc không hề đăng ký để được give() đánh thức. */
static bool waiter_blocked_correctly(const rtos_tcb_t *tcb)
{
    return (tcb->state == RTOS_TASK_BLOCKED) &&
           (tcb->ready_item.container == NULL) &&
           (tcb->wait_item.container == &sem.waiting_tasks);
}

/* Chỉ cần block một lần rồi ghi lại kết quả - phần còn lại của test không
 * cần task này làm gì thêm. */
static void task_waiter_high(void *argument)
{
    (void)argument;

    g_waiter_high_take_status = rtos_sem_take(&sem, RTOS_WAIT_FOREVER);

    for (;;)
    {
        rtos_delay(WAITER_ALIVE_DELAY_TICKS);
    }
}

/* Cùng vai trò với task_waiter_high, chỉ khác priority - dùng để chứng minh
 * give() chọn đúng waiter theo priority chứ không phải theo thứ tự vào wait
 * list. */
static void task_waiter_low(void *argument)
{
    (void)argument;

    g_waiter_low_take_status = rtos_sem_take(&sem, RTOS_WAIT_FOREVER);

    for (;;)
    {
        rtos_delay(WAITER_ALIVE_DELAY_TICKS);
    }
}

/* Conductor dùng priority cao nhất trong toàn bộ file để không ai chen ngang
 * được giữa hai bước kiểm tra. */
static void task_conductor(void *argument)
{
    rtos_status_t status;

    (void)argument;

    bsp_uart_write_string("[semaphore] test_semaphore_blocking: bat dau\r\n");

    /* count == 0 ngay từ đầu - cả hai waiter phải block ngay khi bắt đầu chạy. */
    rtos_sem_init(&sem, 0U, 1U);

    /* Waiter chỉ có cơ hội chạy khi conductor tự nhường CPU như thế này. */
    rtos_delay(SETTLE_TICKS);

    uart_write_pass_fail("both_waiters_blocked",
        waiter_blocked_correctly(&task_waiter_high_tcb) &&
        waiter_blocked_correctly(&task_waiter_low_tcb) &&
        (sem.waiting_tasks.length == 2U));

    /* waiter_high vào wait list sau waiter_low (thứ tự tạo task) nhưng có
     * priority cao hơn - give() phải chọn nó trước. Token được trao thẳng cho
     * waiter_high nên count phải giữ nguyên 0. */
    status = rtos_sem_give(&sem);
    uart_write_pass_fail("give_wakes_high_priority_waiter_first_direct_handoff",
        (status == RTOS_OK) && (sem.count == 0U) &&
        (task_waiter_high_tcb.state == RTOS_TASK_READY) &&
        (task_waiter_high_tcb.ready_item.container == &ready_lists[HIGH_PRIORITY]) &&
        waiter_blocked_correctly(&task_waiter_low_tcb));

    /* waiter_low là waiter duy nhất còn lại nên lần give() thứ hai phải chọn
     * đúng nó. */
    status = rtos_sem_give(&sem);
    uart_write_pass_fail("give_wakes_low_priority_waiter",
        (status == RTOS_OK) && (sem.count == 0U) &&
        (task_waiter_low_tcb.state == RTOS_TASK_READY) &&
        (sem.waiting_tasks.length == 0U));

    /* Nhường CPU để cả hai waiter thực sự resume và tự ghi lại status trả về
     * từ take() của chúng. */
    rtos_delay(SETTLE_TICKS);

    uart_write_pass_fail("both_waiters_resumed_take_returned_ok",
        (g_waiter_high_take_status == RTOS_OK) && (g_waiter_low_take_status == RTOS_OK));

    bsp_uart_write_string("[semaphore] test_semaphore_blocking: xong\r\n");

    for (;;)
    {
        rtos_delay(HEARTBEAT_TICKS);
    }
}

int main(void)
{
    rtos_status_t status_conductor;
    rtos_status_t status_high;
    rtos_status_t status_low;

    board_init();
    rtos_init();

    status_conductor = rtos_task_create_static(&task_conductor_tcb, "conductor", task_conductor,
        NULL, task_conductor_stack, STACK_WORDS, CONDUCTOR_PRIORITY);
    status_high = rtos_task_create_static(&task_waiter_high_tcb, "waiter_high", task_waiter_high,
        NULL, task_waiter_high_stack, STACK_WORDS, HIGH_PRIORITY);
    status_low = rtos_task_create_static(&task_waiter_low_tcb, "waiter_low", task_waiter_low,
        NULL, task_waiter_low_stack, STACK_WORDS, LOW_PRIORITY);

    if ((status_conductor != RTOS_OK) || (status_high != RTOS_OK) || (status_low != RTOS_OK))
    {
        for (;;)
        {
        }
    }

    rtos_start(&task_conductor_tcb, board_cpu_clock_hz());

    for (;;)
    {
    }
}
