/*
 * Queue blocking senders: two senders block on a pre-filled queue, receive()
 * must free a slot and wake them in priority order, not registration order.
 *
 * queue_c phải được mồi đầy trong main(), trước ntos_start() - không phải bên
 * trong conductor. Ngay khi conductor gọi ntos_delay() lần đầu, cả hai sender
 * (priority cao hơn conductor... không, priority thấp hơn conductor nhưng cao
 * hơn nhau) có thể được scheduler chạy lần đầu tiên. Nếu queue_c chưa đầy vào
 * đúng lúc đó, sender sẽ gửi thành công ngay thay vì block, và toàn bộ phần
 * kiểm tra blocking sender phía sau sẽ dựa trên một tiền đề sai.
 *
 * TEST-ONLY: include thẳng kernel/internal để đọc ready_lists[] (xem
 * NTOS_CODING_CONVENTION.md mục 2.4).
 */
#include <ntos/ntos.h>
#include <board.h>
#include <uart.h>
#include "ntos_kernel_internal.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define STACK_WORDS ((size_t)64U)

#define CONDUCTOR_PRIORITY      ((ntos_priority_t)7U)
#define SENDER_HIGH_PRIORITY    ((ntos_priority_t)6U)
#define SENDER_LOW_PRIORITY     ((ntos_priority_t)3U)

#define SETTLE_TICKS ((ntos_tick_t)5U)
#define HEARTBEAT_TICKS ((ntos_tick_t)2000U)
#define TASK_ALIVE_DELAY_TICKS ((ntos_tick_t)1000U)

#define QUEUE_C_PRIME_VALUE ((uint32_t)0xFFFFFFFFU)
#define SENDER_HIGH_VALUE ((uint32_t)0xAAAAAAAAU)
#define SENDER_LOW_VALUE  ((uint32_t)0xBBBBBBBBU)

static ntos_tcb_t task_conductor_tcb;
static ntos_tcb_t task_sender_high_tcb;
static ntos_tcb_t task_sender_low_tcb;

static _Alignas(8) uint32_t task_conductor_stack[STACK_WORDS];
static _Alignas(8) uint32_t task_sender_high_stack[STACK_WORDS];
static _Alignas(8) uint32_t task_sender_low_stack[STACK_WORDS];

static ntos_queue_t queue_c;
static uint32_t queue_c_storage[1];

static volatile ntos_status_t g_sender_high_status = NTOS_ERR_INTERNAL;
static volatile ntos_status_t g_sender_low_status = NTOS_ERR_INTERNAL;

static void uart_write_pass_fail(const char *name, bool ok)
{
    bsp_uart_write_string("[queue] ");
    bsp_uart_write_string(name);
    bsp_uart_write_string(ok ? ": PASS\r\n" : ": FAIL\r\n");
}

/* Đối xứng với receiver_blocked_correctly() ở blocking_receive.c: kiểm cả
 * state lẫn việc wait_item đã gắn đúng vào waiting_senders. */
static bool sender_blocked_correctly(const ntos_tcb_t *tcb)
{
    return (tcb->state == NTOS_TASK_BLOCKED) &&
           (tcb->ready_item.container == NULL) &&
           (tcb->wait_item.container == &queue_c.waiting_senders);
}

static void task_conductor(void *argument)
{
    ntos_status_t status;
    uint32_t value;

    (void)argument;

    bsp_uart_write_string("[queue] test_queue_blocking_send: bat dau\r\n");

    /* queue_c đã được mồi đầy trong main() từ trước ntos_start(), nên cả hai
     * sender thấy queue đầy ngay từ lần gửi đầu tiên và tự block - việc đó đã
     * xảy ra trong lần nhường CPU đầu tiên dưới đây. */
    ntos_delay(SETTLE_TICKS);

    uart_write_pass_fail("both_senders_blocked_on_queue_c",
        sender_blocked_correctly(&task_sender_high_tcb) &&
        sender_blocked_correctly(&task_sender_low_tcb) &&
        (queue_c.waiting_senders.length == 2U) && (queue_c.count == 1U));

    /* receive() giải phóng đúng một chỗ trống - sender_high có priority cao
     * hơn sender_low nên phải được chọn để lấp vào chỗ đó trước. Giá trị nhận
     * được phải là giá trị đã mồi từ main(), chứng minh receive() lấy đúng
     * item có sẵn trong buffer chứ không lẫn với item của sender vừa wake. */
    status = ntos_queue_receive(&queue_c, &value, NTOS_NO_WAIT);
    uart_write_pass_fail("receive_returns_primed_value_and_wakes_highest_priority_sender",
        (status == NTOS_OK) && (value == QUEUE_C_PRIME_VALUE) &&
        (task_sender_high_tcb.state == NTOS_TASK_READY) &&
        (task_sender_high_tcb.ready_item.container == &ready_lists[SENDER_HIGH_PRIORITY]) &&
        sender_blocked_correctly(&task_sender_low_tcb) &&
        (queue_c.waiting_senders.length == 1U));

    /* Nhường CPU để sender_high resume và tự ghi item của nó vào chỗ trống
     * vừa giải phóng - kiểm count quay lại 1 mới chứng minh item đã được ghi. */
    ntos_delay(SETTLE_TICKS);

    uart_write_pass_fail("high_sender_resumed_and_sent_value",
        (g_sender_high_status == NTOS_OK) && (queue_c.count == 1U));

    /* Item hiện có trong buffer phải là của sender_high, và giải phóng chỗ
     * trống lần này phải đến lượt sender_low. */
    status = ntos_queue_receive(&queue_c, &value, NTOS_NO_WAIT);
    uart_write_pass_fail("receive_returns_high_sender_value_then_wakes_low_sender",
        (status == NTOS_OK) && (value == SENDER_HIGH_VALUE) &&
        (task_sender_low_tcb.state == NTOS_TASK_READY) &&
        (queue_c.waiting_senders.length == 0U));

    ntos_delay(SETTLE_TICKS);

    uart_write_pass_fail("low_sender_resumed_and_sent_value",
        (g_sender_low_status == NTOS_OK) && (queue_c.count == 1U));

    /* Item cuối cùng trong buffer phải là của sender_low - xác nhận toàn bộ
     * chuỗi handoff-qua-buffer đã đi đúng thứ tự từ đầu tới cuối. */
    status = ntos_queue_receive(&queue_c, &value, NTOS_NO_WAIT);
    uart_write_pass_fail("final_receive_returns_low_sender_value",
        (status == NTOS_OK) && (value == SENDER_LOW_VALUE));

    bsp_uart_write_string("[queue] test_queue_blocking_send: xong\r\n");

    for (;;)
    {
        ntos_delay(HEARTBEAT_TICKS);
    }
}

/* Gửi đúng một item (giá trị cố định, khác với sender_low, để phân biệt được
 * trong buffer item nào tới từ ai) rồi ghi lại status. */
static void task_sender_high(void *argument)
{
    ntos_status_t status;
    uint32_t value = SENDER_HIGH_VALUE;

    (void)argument;

    status = ntos_queue_send(&queue_c, &value, NTOS_WAIT_FOREVER);
    g_sender_high_status = status;

    for (;;)
    {
        ntos_delay(TASK_ALIVE_DELAY_TICKS);
    }
}

/* Cùng vai trò với task_sender_high, chỉ khác priority. */
static void task_sender_low(void *argument)
{
    ntos_status_t status;
    uint32_t value = SENDER_LOW_VALUE;

    (void)argument;

    status = ntos_queue_send(&queue_c, &value, NTOS_WAIT_FOREVER);
    g_sender_low_status = status;

    for (;;)
    {
        ntos_delay(TASK_ALIVE_DELAY_TICKS);
    }
}

int main(void)
{
    ntos_status_t status_queue_c;
    ntos_status_t status_prime;
    uint32_t prime_value;
    ntos_status_t status_conductor;
    ntos_status_t status_sender_high;
    ntos_status_t status_sender_low;

    board_init();
    ntos_init();

    status_queue_c = ntos_queue_init_static(&queue_c, queue_c_storage, sizeof(uint32_t), 1U);

    /* Mồi đầy queue_c NGAY TỪ ĐÂY (trước khi tạo task/ntos_start()) - xem giải
     * thích thứ tự init ở đầu file. */
    prime_value = QUEUE_C_PRIME_VALUE;
    status_prime = ntos_queue_send(&queue_c, &prime_value, NTOS_NO_WAIT);

    if ((status_queue_c != NTOS_OK) || (status_prime != NTOS_OK))
    {
        for (;;)
        {
        }
    }

    status_conductor = ntos_task_create_static(&task_conductor_tcb, "conductor", task_conductor,
        NULL, task_conductor_stack, STACK_WORDS, CONDUCTOR_PRIORITY);
    status_sender_high = ntos_task_create_static(&task_sender_high_tcb, "sender_high", task_sender_high,
        NULL, task_sender_high_stack, STACK_WORDS, SENDER_HIGH_PRIORITY);
    status_sender_low = ntos_task_create_static(&task_sender_low_tcb, "sender_low", task_sender_low,
        NULL, task_sender_low_stack, STACK_WORDS, SENDER_LOW_PRIORITY);

    if ((status_conductor != NTOS_OK) || (status_sender_high != NTOS_OK) ||
        (status_sender_low != NTOS_OK))
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
