/*
 * Queue blocking receivers: two receivers block on an empty queue, send()
 * must wake them in priority order, not registration order.
 *
 * TEST-ONLY: include thẳng kernel/src/kernel_internal.h để đọc ready_lists[] (xem
 * RTOS_CODING_CONVENTION.md mục 2.4) - rtos_queue_t/rtos_tcb_t đã public hoàn
 * toàn nên không cần internal header cho phần còn lại.
 */
#include <rtos.h>
#include <bsp.h>
#include "kernel_internal.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define STACK_WORDS ((size_t)64U)

#define CONDUCTOR_PRIORITY      ((rtos_priority_t)7U)
#define RECEIVER_HIGH_PRIORITY  ((rtos_priority_t)5U)
#define RECEIVER_LOW_PRIORITY   ((rtos_priority_t)2U)

#define SETTLE_TICKS ((rtos_tick_t)5U)
#define HEARTBEAT_TICKS ((rtos_tick_t)2000U)
#define TASK_ALIVE_DELAY_TICKS ((rtos_tick_t)1000U)

static rtos_tcb_t task_conductor_tcb;
static rtos_tcb_t task_receiver_high_tcb;
static rtos_tcb_t task_receiver_low_tcb;

static _Alignas(8) uint32_t task_conductor_stack[STACK_WORDS];
static _Alignas(8) uint32_t task_receiver_high_stack[STACK_WORDS];
static _Alignas(8) uint32_t task_receiver_low_stack[STACK_WORDS];

static rtos_queue_t queue_b;
static uint32_t queue_b_storage[1];

static volatile rtos_status_t g_receiver_high_status = RTOS_ERR_INTERNAL;
static volatile uint32_t g_receiver_high_value;
static volatile rtos_status_t g_receiver_low_status = RTOS_ERR_INTERNAL;
static volatile uint32_t g_receiver_low_value;

static void uart_write_pass_fail(const char *name, bool ok)
{
    bsp_uart_write_string("[queue] ");
    bsp_uart_write_string(name);
    bsp_uart_write_string(ok ? ": PASS\r\n" : ": FAIL\r\n");
}

/* Kiểm cả state lẫn hai list-membership: chỉ kiểm state == BLOCKED không đủ
 * để chứng minh task đã thật sự rời ready list và gắn đúng vào
 * waiting_receivers. */
static bool receiver_blocked_correctly(const rtos_tcb_t *tcb)
{
    return (tcb->state == RTOS_TASK_BLOCKED) &&
           (tcb->ready_item.container == NULL) &&
           (tcb->wait_item.container == &queue_b.waiting_receivers);
}

/* Conductor dùng priority cao nhất để không ai chen ngang được giữa các bước
 * kiểm tra: hai receiver chỉ có cơ hội chạy tại các điểm rtos_delay() mà
 * conductor chủ động chọn bên dưới. */
static void task_conductor(void *argument)
{
    rtos_status_t status;
    uint32_t value;

    (void)argument;

    bsp_uart_write_string("[queue] test_queue_blocking_receive: bat dau\r\n");

    /* Cả hai receiver gọi rtos_queue_receive(WAIT_FOREVER) ngay khi bắt đầu
     * chạy, và queue_b rỗng - nhường CPU một lần là đủ để cả hai tự đưa mình
     * vào wait list trước khi kiểm tra bên dưới đọc lại trạng thái. */
    rtos_delay(SETTLE_TICKS);

    uart_write_pass_fail("both_receivers_blocked_on_queue_b",
        receiver_blocked_correctly(&task_receiver_high_tcb) &&
        receiver_blocked_correctly(&task_receiver_low_tcb) &&
        (queue_b.waiting_receivers.length == 2U));

    /* receiver_high vào wait list sau receiver_low (thứ tự tạo task) nhưng có
     * priority cao hơn - send() phải wake nó trước. */
    value = 111U;
    status = rtos_queue_send(&queue_b, &value, RTOS_NO_WAIT);
    uart_write_pass_fail("send_wakes_highest_priority_receiver_first",
        (status == RTOS_OK) &&
        (task_receiver_high_tcb.state == RTOS_TASK_READY) &&
        (task_receiver_high_tcb.ready_item.container == &ready_lists[RECEIVER_HIGH_PRIORITY]) &&
        receiver_blocked_correctly(&task_receiver_low_tcb) &&
        (queue_b.waiting_receivers.length == 1U));

    /* Nhường CPU để receiver_high thực sự resume và tự copy item ra khỏi
     * buffer - kiểm giá trị nhận được, không chỉ status OK. */
    rtos_delay(SETTLE_TICKS);

    uart_write_pass_fail("high_receiver_resumed_and_received_value",
        (g_receiver_high_status == RTOS_OK) && (g_receiver_high_value == 111U) &&
        (queue_b.count == 0U));

    /* Chỉ còn receiver_low trong wait list - send() lần này phải chọn đúng nó. */
    value = 222U;
    status = rtos_queue_send(&queue_b, &value, RTOS_NO_WAIT);
    uart_write_pass_fail("send_wakes_remaining_low_priority_receiver",
        (status == RTOS_OK) &&
        (task_receiver_low_tcb.state == RTOS_TASK_READY) &&
        (queue_b.waiting_receivers.length == 0U));

    rtos_delay(SETTLE_TICKS);

    uart_write_pass_fail("low_receiver_resumed_and_received_value",
        (g_receiver_low_status == RTOS_OK) && (g_receiver_low_value == 222U) &&
        (queue_b.count == 0U));

    bsp_uart_write_string("[queue] test_queue_blocking_receive: xong\r\n");

    for (;;)
    {
        rtos_delay(HEARTBEAT_TICKS);
    }
}

/* Nhận đúng một item rồi ghi lại status/giá trị để conductor kiểm tra. */
static void task_receiver_high(void *argument)
{
    rtos_status_t status;
    uint32_t value = 0U;

    (void)argument;

    status = rtos_queue_receive(&queue_b, &value, RTOS_WAIT_FOREVER);
    g_receiver_high_status = status;
    g_receiver_high_value = value;

    for (;;)
    {
        rtos_delay(TASK_ALIVE_DELAY_TICKS);
    }
}

/* Cùng vai trò với task_receiver_high, chỉ khác priority - dùng để chứng
 * minh send() chọn waiter theo priority chứ không theo thứ tự đăng ký chờ. */
static void task_receiver_low(void *argument)
{
    rtos_status_t status;
    uint32_t value = 0U;

    (void)argument;

    status = rtos_queue_receive(&queue_b, &value, RTOS_WAIT_FOREVER);
    g_receiver_low_status = status;
    g_receiver_low_value = value;

    for (;;)
    {
        rtos_delay(TASK_ALIVE_DELAY_TICKS);
    }
}

int main(void)
{
    rtos_status_t status_queue_b;
    rtos_status_t status_conductor;
    rtos_status_t status_receiver_high;
    rtos_status_t status_receiver_low;

    board_init();
    rtos_init();

    status_queue_b = rtos_queue_init_static(&queue_b, queue_b_storage, sizeof(uint32_t), 1U);

    if (status_queue_b != RTOS_OK)
    {
        for (;;)
        {
        }
    }

    status_conductor = rtos_task_create_static(&task_conductor_tcb, "conductor", task_conductor,
        NULL, task_conductor_stack, STACK_WORDS, CONDUCTOR_PRIORITY);
    status_receiver_high = rtos_task_create_static(&task_receiver_high_tcb, "receiver_high", task_receiver_high,
        NULL, task_receiver_high_stack, STACK_WORDS, RECEIVER_HIGH_PRIORITY);
    status_receiver_low = rtos_task_create_static(&task_receiver_low_tcb, "receiver_low", task_receiver_low,
        NULL, task_receiver_low_stack, STACK_WORDS, RECEIVER_LOW_PRIORITY);

    if ((status_conductor != RTOS_OK) || (status_receiver_high != RTOS_OK) ||
        (status_receiver_low != RTOS_OK))
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
