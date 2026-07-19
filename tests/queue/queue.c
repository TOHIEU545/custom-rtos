/*
 * Integration test for queue behavior in task context.
 *
 * Verifies ring-buffer FIFO behavior, full/empty handling, index wrap-around,
 * blocking receivers, blocking senders, priority-based waiter selection and
 * condition re-check after wake-up.
 *
 * queue_c phải được mồi đầy trong main(), trước ntos_start() - không phải bên
 * trong conductor. Ngay khi conductor gọi ntos_delay() lần đầu, mọi task còn
 * lại (kể cả sender_high/sender_low, priority cao hơn cả hai receiver) đều có
 * thể được scheduler chạy lần đầu tiên, không theo thứ tự các hàm xuất hiện
 * trong file này. Nếu queue_c chưa đầy vào đúng lúc đó, sender sẽ gửi thành
 * công ngay thay vì block, và toàn bộ phần kiểm tra blocking sender phía sau
 * sẽ dựa trên một tiền đề sai.
 *
 * TEST-ONLY: include thẳng kernel/internal để đọc ready_lists[] (xem
 * NTOS_CODING_CONVENTION.md mục 2.4) - ntos_queue_t/ntos_tcb_t đã public hoàn
 * toàn nên không cần internal header cho phần còn lại.
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
#define RECEIVER_HIGH_PRIORITY  ((ntos_priority_t)5U)
#define SENDER_LOW_PRIORITY     ((ntos_priority_t)3U)
#define RECEIVER_LOW_PRIORITY   ((ntos_priority_t)2U)

#define SETTLE_TICKS ((ntos_tick_t)5U)
#define HEARTBEAT_TICKS ((ntos_tick_t)2000U)
#define TASK_ALIVE_DELAY_TICKS ((ntos_tick_t)1000U)

#define QUEUE_C_PRIME_VALUE ((uint32_t)0xFFFFFFFFU)
#define SENDER_HIGH_VALUE ((uint32_t)0xAAAAAAAAU)
#define SENDER_LOW_VALUE  ((uint32_t)0xBBBBBBBBU)

static ntos_tcb_t task_conductor_tcb;
static ntos_tcb_t task_receiver_high_tcb;
static ntos_tcb_t task_receiver_low_tcb;
static ntos_tcb_t task_sender_high_tcb;
static ntos_tcb_t task_sender_low_tcb;

static _Alignas(8) uint32_t task_conductor_stack[STACK_WORDS];
static _Alignas(8) uint32_t task_receiver_high_stack[STACK_WORDS];
static _Alignas(8) uint32_t task_receiver_low_stack[STACK_WORDS];
static _Alignas(8) uint32_t task_sender_high_stack[STACK_WORDS];
static _Alignas(8) uint32_t task_sender_low_stack[STACK_WORDS];

static ntos_queue_t queue_a;
static uint32_t queue_a_storage[2];

static ntos_queue_t queue_b;
static uint32_t queue_b_storage[1];

static ntos_queue_t queue_c;
static uint32_t queue_c_storage[1];

static volatile ntos_status_t g_receiver_high_status = NTOS_ERR_INTERNAL;
static volatile uint32_t g_receiver_high_value;
static volatile ntos_status_t g_receiver_low_status = NTOS_ERR_INTERNAL;
static volatile uint32_t g_receiver_low_value;

static volatile ntos_status_t g_sender_high_status = NTOS_ERR_INTERNAL;
static volatile ntos_status_t g_sender_low_status = NTOS_ERR_INTERNAL;

static void uart_write_pass_fail(const char *name, bool ok)
{
    bsp_uart_write_string("[9B] ");
    bsp_uart_write_string(name);
    bsp_uart_write_string(ok ? ": PASS\r\n" : ": FAIL\r\n");
}

/* Ring buffer thuần, chạy tuần tự trong conductor bằng toàn NTOS_NO_WAIT -
 * không cần task nào khác vì mọi thao tác đều thành công hoặc thất bại ngay
 * lập tức, không có nhánh nào cần block. */
static void conductor_run_test_a(void)
{
    ntos_status_t status;
    uint32_t value;

    value = 10U;
    status = ntos_queue_send(&queue_a, &value, NTOS_NO_WAIT);
    uart_write_pass_fail("send_first_item_ok", (status == NTOS_OK) && (queue_a.count == 1U));

    value = 20U;
    status = ntos_queue_send(&queue_a, &value, NTOS_NO_WAIT);
    uart_write_pass_fail("send_second_item_fills_queue_and_wraps_write_index",
        (status == NTOS_OK) && (queue_a.count == 2U) && (queue_a.write_index == 0U));

    value = 30U;
    status = ntos_queue_send(&queue_a, &value, NTOS_NO_WAIT);
    uart_write_pass_fail("send_no_wait_on_full_returns_queue_full",
        (status == NTOS_ERR_QUEUE_FULL) && (queue_a.count == 2U));

    value = 0U;
    status = ntos_queue_receive(&queue_a, &value, NTOS_NO_WAIT);
    uart_write_pass_fail("receive_returns_fifo_first_item",
        (status == NTOS_OK) && (value == 10U) && (queue_a.count == 1U));

    status = ntos_queue_receive(&queue_a, &value, NTOS_NO_WAIT);
    uart_write_pass_fail("receive_returns_fifo_second_item_and_wraps_read_index",
        (status == NTOS_OK) && (value == 20U) && (queue_a.count == 0U) && (queue_a.read_index == 0U));

    status = ntos_queue_receive(&queue_a, &value, NTOS_NO_WAIT);
    uart_write_pass_fail("receive_no_wait_on_empty_returns_queue_empty",
        status == NTOS_ERR_QUEUE_EMPTY);

    /* Gửi thêm sau khi cả hai index đã wrap về 0 một lần - kiểm wrap ổn định
     * qua nhiều vòng, không chỉ đúng lần đầu. */
    value = 40U;
    (void)ntos_queue_send(&queue_a, &value, NTOS_NO_WAIT);
    value = 50U;
    status = ntos_queue_send(&queue_a, &value, NTOS_NO_WAIT);
    uart_write_pass_fail("send_after_wrap_still_ok", (status == NTOS_OK) && (queue_a.count == 2U));

    status = ntos_queue_receive(&queue_a, &value, NTOS_NO_WAIT);
    uart_write_pass_fail("receive_after_wrap_fifo_correct_first", (status == NTOS_OK) && (value == 40U));

    status = ntos_queue_receive(&queue_a, &value, NTOS_NO_WAIT);
    uart_write_pass_fail("receive_after_wrap_fifo_correct_second", (status == NTOS_OK) && (value == 50U));
}

/* Kiểm cả state lẫn hai list-membership: chỉ kiểm state == BLOCKED không đủ
 * để chứng minh task đã thật sự rời ready list và gắn đúng vào
 * waiting_receivers - hai thao tác đó phải xảy ra cùng nhau, một bug chỉ cập
 * nhật state mà quên list vẫn có thể lọt qua nếu chỉ kiểm state. */
static bool receiver_blocked_correctly(const ntos_tcb_t *tcb)
{
    return (tcb->state == NTOS_TASK_BLOCKED) &&
           (tcb->ready_item.container == NULL) &&
           (tcb->wait_item.container == &queue_b.waiting_receivers);
}

static void conductor_run_test_b(void)
{
    ntos_status_t status;
    uint32_t value;

    /* Conductor có priority cao nhất nên receiver_high/receiver_low/
     * sender_high/sender_low chỉ có cơ hội chạy khi nó tự nhường CPU như thế
     * này - cả bốn gọi API blocking ngay khi bắt đầu chạy, nên nhường CPU một
     * lần ở đây là đủ để tất cả tự đưa mình vào đúng wait list trước khi các
     * bước kiểm tra bên dưới đọc lại trạng thái của chúng. */
    ntos_delay(SETTLE_TICKS);

    uart_write_pass_fail("both_receivers_blocked_on_queue_b",
        receiver_blocked_correctly(&task_receiver_high_tcb) &&
        receiver_blocked_correctly(&task_receiver_low_tcb) &&
        (queue_b.waiting_receivers.length == 2U));

    /* receiver_high vào wait list sau receiver_low (thứ tự tạo task) nhưng có
     * priority cao hơn - send() phải wake nó trước để chứng minh việc chọn
     * waiter dựa trên priority chứ không phải thứ tự đăng ký chờ. */
    value = 111U;
    status = ntos_queue_send(&queue_b, &value, NTOS_NO_WAIT);
    uart_write_pass_fail("send_wakes_highest_priority_receiver_first",
        (status == NTOS_OK) &&
        (task_receiver_high_tcb.state == NTOS_TASK_READY) &&
        (task_receiver_high_tcb.ready_item.container == &ready_lists[RECEIVER_HIGH_PRIORITY]) &&
        receiver_blocked_correctly(&task_receiver_low_tcb) &&
        (queue_b.waiting_receivers.length == 1U));

    /* send() ở trên chỉ chuyển receiver_high sang READY, chưa chắc nó đã
     * thực sự chạy lại và lấy item (conductor vẫn priority cao hơn). Nhường
     * CPU để nó resume và tự copy item ra khỏi buffer - kiểm giá trị nhận
     * được (không chỉ status OK) mới chứng minh được việc "được wake" đã
     * thật sự dẫn tới một lần receive thành công, không phải trùng hợp còn
     * giá trị cũ trong biến. */
    ntos_delay(SETTLE_TICKS);

    uart_write_pass_fail("high_receiver_resumed_and_received_value",
        (g_receiver_high_status == NTOS_OK) && (g_receiver_high_value == 111U) &&
        (queue_b.count == 0U));

    /* receiver_high đã nhận item và không gọi receive() lần hai (nó chỉ
     * delay-loop sau khi resume), nên giờ chỉ còn receiver_low trong wait
     * list - send() lần này phải chọn đúng nó. */
    value = 222U;
    status = ntos_queue_send(&queue_b, &value, NTOS_NO_WAIT);
    uart_write_pass_fail("send_wakes_remaining_low_priority_receiver",
        (status == NTOS_OK) &&
        (task_receiver_low_tcb.state == NTOS_TASK_READY) &&
        ntos_list_is_empty(&queue_b.waiting_receivers));

    ntos_delay(SETTLE_TICKS);

    uart_write_pass_fail("low_receiver_resumed_and_received_value",
        (g_receiver_low_status == NTOS_OK) && (g_receiver_low_value == 222U) &&
        (queue_b.count == 0U));
}

/* Đối xứng với receiver_blocked_correctly(): kiểm cả state lẫn việc wait_item
 * đã gắn đúng vào waiting_senders, không chỉ state. */
static bool sender_blocked_correctly(const ntos_tcb_t *tcb)
{
    return (tcb->state == NTOS_TASK_BLOCKED) &&
           (tcb->ready_item.container == NULL) &&
           (tcb->wait_item.container == &queue_c.waiting_senders);
}

static void conductor_run_test_c(void)
{
    ntos_status_t status;
    uint32_t value;

    /* queue_c đã được mồi đầy trong main() từ trước ntos_start(), nên
     * sender_high/sender_low thấy queue đầy ngay từ lần gửi đầu tiên và tự
     * block - việc đó đã xảy ra trong lần nhường CPU đầu tiên của
     * conductor_run_test_b() (mọi task khác đều chạy lần đầu ở đó), nên
     * không cần nhường CPU thêm ở đây trước khi kiểm tra. */
    uart_write_pass_fail("both_senders_blocked_on_queue_c",
        sender_blocked_correctly(&task_sender_high_tcb) &&
        sender_blocked_correctly(&task_sender_low_tcb) &&
        (queue_c.waiting_senders.length == 2U) && (queue_c.count == 1U));

    /* receive() giải phóng đúng một chỗ trống - sender_high có priority cao
     * hơn sender_low nên phải được chọn để lấp vào chỗ đó trước, bất kể thứ
     * tự hai sender đã block trước đó. Giá trị nhận được phải là giá trị đã
     * mồi từ main(), chứng minh receive() lấy đúng item có sẵn trong buffer
     * chứ không lẫn với item của sender vừa được wake (sender đó chưa kịp
     * ghi gì). */
    status = ntos_queue_receive(&queue_c, &value, NTOS_NO_WAIT);
    uart_write_pass_fail("receive_returns_primed_value_and_wakes_highest_priority_sender",
        (status == NTOS_OK) && (value == QUEUE_C_PRIME_VALUE) &&
        (task_sender_high_tcb.state == NTOS_TASK_READY) &&
        (task_sender_high_tcb.ready_item.container == &ready_lists[SENDER_HIGH_PRIORITY]) &&
        sender_blocked_correctly(&task_sender_low_tcb) &&
        (queue_c.waiting_senders.length == 1U));

    /* receive() ở trên chỉ chuyển sender_high sang READY, chưa chắc nó đã
     * thực sự chạy lại để ghi item của nó vào chỗ trống vừa giải phóng.
     * Nhường CPU để nó resume và tự gửi - kiểm count quay lại 1 mới chứng
     * minh được item thật sự đã được ghi, không chỉ trạng thái task đổi. */
    ntos_delay(SETTLE_TICKS);

    uart_write_pass_fail("high_sender_resumed_and_sent_value",
        (g_sender_high_status == NTOS_OK) && (queue_c.count == 1U));

    /* Item hiện có trong buffer phải là của sender_high (vừa ghi ở bước
     * trên), và giải phóng chỗ trống lần này phải đến lượt sender_low - waiter
     * priority cao hơn duy nhất còn lại đã rời wait list ở bước trước. */
    status = ntos_queue_receive(&queue_c, &value, NTOS_NO_WAIT);
    uart_write_pass_fail("receive_returns_high_sender_value_then_wakes_low_sender",
        (status == NTOS_OK) && (value == SENDER_HIGH_VALUE) &&
        (task_sender_low_tcb.state == NTOS_TASK_READY) &&
        ntos_list_is_empty(&queue_c.waiting_senders));

    ntos_delay(SETTLE_TICKS);

    uart_write_pass_fail("low_sender_resumed_and_sent_value",
        (g_sender_low_status == NTOS_OK) && (queue_c.count == 1U));

    /* Item cuối cùng trong buffer phải là của sender_low - xác nhận toàn bộ
     * chuỗi handoff-qua-buffer (không phải direct handoff) đã đi đúng thứ tự
     * từ đầu tới cuối. */
    status = ntos_queue_receive(&queue_c, &value, NTOS_NO_WAIT);
    uart_write_pass_fail("final_receive_returns_low_sender_value",
        (status == NTOS_OK) && (value == SENDER_LOW_VALUE));
}

/* Conductor dùng priority cao nhất trong toàn bộ file để không ai chen ngang
 * được giữa hai bước kiểm tra: bốn task còn lại chỉ có cơ hội chạy tại các
 * điểm ntos_delay() mà conductor chủ động chọn bên trong ba hàm dưới đây, nên
 * thứ tự quan sát được là xác định thay vì phụ thuộc scheduler tình cờ chọn
 * ai chạy trước. */
static void task_conductor(void *argument)
{
    (void)argument;

    bsp_uart_write_string("[queue] test_queue: bat dau\r\n");

    conductor_run_test_a();
    conductor_run_test_b();
    conductor_run_test_c();

    bsp_uart_write_string("[queue] test_queue: xong\r\n");

    for (;;)
    {
        ntos_delay(HEARTBEAT_TICKS);
    }
}

/* Nhận đúng một item rồi ghi lại status/giá trị để conductor kiểm tra - phần
 * còn lại của test không cần task này làm gì thêm. Vòng lặp delay giữ task
 * tồn tại mà không chiếm CPU (conductor vẫn priority cao hơn nên hầu như
 * không nhường CPU cho nó nữa sau lần resume này). */
static void task_receiver_high(void *argument)
{
    ntos_status_t status;
    uint32_t value = 0U;

    (void)argument;

    status = ntos_queue_receive(&queue_b, &value, NTOS_WAIT_FOREVER);
    g_receiver_high_status = status;
    g_receiver_high_value = value;

    for (;;)
    {
        ntos_delay(TASK_ALIVE_DELAY_TICKS);
    }
}

/* Cùng vai trò với task_receiver_high, chỉ khác priority - dùng để chứng
 * minh send() chọn waiter theo priority chứ không theo thứ tự đăng ký chờ. */
static void task_receiver_low(void *argument)
{
    ntos_status_t status;
    uint32_t value = 0U;

    (void)argument;

    status = ntos_queue_receive(&queue_b, &value, NTOS_WAIT_FOREVER);
    g_receiver_low_status = status;
    g_receiver_low_value = value;

    for (;;)
    {
        ntos_delay(TASK_ALIVE_DELAY_TICKS);
    }
}

/* Gửi đúng một item (giá trị cố định, khác với sender_low, để phân biệt được
 * trong buffer item nào tới từ ai) rồi ghi lại status - đối xứng với
 * task_receiver_high nhưng ở chiều gửi. */
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
    ntos_status_t status_queue_a;
    ntos_status_t status_queue_b;
    ntos_status_t status_queue_c;
    ntos_status_t status_prime;
    uint32_t prime_value;
    ntos_status_t status_conductor;
    ntos_status_t status_receiver_high;
    ntos_status_t status_receiver_low;
    ntos_status_t status_sender_high;
    ntos_status_t status_sender_low;

    board_init();
    ntos_init();

    status_queue_a = ntos_queue_init_static(&queue_a, queue_a_storage, sizeof(uint32_t), 2U);
    status_queue_b = ntos_queue_init_static(&queue_b, queue_b_storage, sizeof(uint32_t), 1U);
    status_queue_c = ntos_queue_init_static(&queue_c, queue_c_storage, sizeof(uint32_t), 1U);

    /* Mồi đầy queue_c NGAY TỪ ĐÂY (trước khi tạo task/ntos_start()) - xem giải
     * thích thứ tự init ở đầu file: sender_high/sender_low phải thấy queue đã
     * đầy ngay lần chạy đầu tiên của chúng. */
    prime_value = QUEUE_C_PRIME_VALUE;
    status_prime = ntos_queue_send(&queue_c, &prime_value, NTOS_NO_WAIT);

    if ((status_queue_a != NTOS_OK) || (status_queue_b != NTOS_OK) ||
        (status_queue_c != NTOS_OK) || (status_prime != NTOS_OK))
    {
        for (;;)
        {
        }
    }

    status_conductor = ntos_task_create_static(&task_conductor_tcb, "conductor", task_conductor,
        NULL, task_conductor_stack, STACK_WORDS, CONDUCTOR_PRIORITY);
    status_receiver_high = ntos_task_create_static(&task_receiver_high_tcb, "receiver_high", task_receiver_high,
        NULL, task_receiver_high_stack, STACK_WORDS, RECEIVER_HIGH_PRIORITY);
    status_receiver_low = ntos_task_create_static(&task_receiver_low_tcb, "receiver_low", task_receiver_low,
        NULL, task_receiver_low_stack, STACK_WORDS, RECEIVER_LOW_PRIORITY);
    status_sender_high = ntos_task_create_static(&task_sender_high_tcb, "sender_high", task_sender_high,
        NULL, task_sender_high_stack, STACK_WORDS, SENDER_HIGH_PRIORITY);
    status_sender_low = ntos_task_create_static(&task_sender_low_tcb, "sender_low", task_sender_low,
        NULL, task_sender_low_stack, STACK_WORDS, SENDER_LOW_PRIORITY);

    if ((status_conductor != NTOS_OK) || (status_receiver_high != NTOS_OK) ||
        (status_receiver_low != NTOS_OK) || (status_sender_high != NTOS_OK) ||
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
