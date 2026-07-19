/*
 * Integration test for semaphore behavior in task context.
 *
 * Verifies immediate take, non-blocking failure, blocking waiters,
 * priority-based wake-up, direct handoff, and maximum-count handling.
 *
 * The conductor task has the highest priority so state checks happen in a
 * deterministic order; waiter tasks run only when the conductor explicitly
 * blocks.
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

/* Kiểm cả state lẫn hai list-membership, không chỉ state: một bug chỉ cập
 * nhật state == BLOCKED mà quên gỡ ready_item hoặc quên gắn wait_item vẫn có
 * thể khiến task trông "đã block" trong khi thật ra vẫn còn trong ready list
 * (bị scheduler chọn nhầm) hoặc không hề đăng ký để được give() đánh thức. */
static bool waiter_blocked_correctly(const ntos_tcb_t *tcb)
{
    return (tcb->state == NTOS_TASK_BLOCKED) &&
           (tcb->ready_item.container == NULL) &&
           (tcb->wait_item.container == &sem.waiting_tasks);
}

/* Chỉ cần block một lần rồi ghi lại kết quả - phần còn lại của test không
 * cần task này làm gì thêm. Vòng lặp delay giữ task tồn tại mà không chiếm
 * CPU (conductor vẫn priority cao hơn nên hầu như không nhường CPU cho nó). */
static void task_waiter_high(void *argument)
{
    (void)argument;

    g_waiter_high_take_status = ntos_sem_take(&sem, NTOS_WAIT_FOREVER);

    for (;;)
    {
        ntos_delay(WAITER_ALIVE_DELAY_TICKS);
    }
}

/* Cùng vai trò với task_waiter_high, chỉ khác priority - dùng để chứng minh
 * give() chọn đúng waiter theo priority chứ không phải theo thứ tự vào wait
 * list. */
static void task_waiter_low(void *argument)
{
    (void)argument;

    g_waiter_low_take_status = ntos_sem_take(&sem, NTOS_WAIT_FOREVER);

    for (;;)
    {
        ntos_delay(WAITER_ALIVE_DELAY_TICKS);
    }
}

/* Conductor dùng priority cao nhất trong toàn bộ file để không ai chen ngang
 * được giữa hai bước kiểm tra: waiter_high/waiter_low chỉ có cơ hội chạy tại
 * các điểm ntos_delay() mà conductor chủ động chọn bên dưới, nên thứ tự quan
 * sát được là xác định thay vì phụ thuộc việc scheduler tình cờ chọn ai chạy
 * trước. */
static void task_conductor(void *argument)
{
    ntos_status_t status;

    (void)argument;

    bsp_uart_write_string("[9A] chang09_semaphore: bat dau\r\n");

    /* count khởi tạo > 0 để bước kiểm tra kế tiếp lấy được ngay mà không phải
     * đi qua nhánh block trước. */
    ntos_sem_init(&sem, 1U, 1U);
    uart_write_pass_fail("init_fields_correct",
        (sem.count == 1U) && (sem.max_count == 1U) && ntos_list_is_empty(&sem.waiting_tasks));

    status = ntos_sem_take(&sem, NTOS_NO_WAIT);
    uart_write_pass_fail("take_success_decrements_count", (status == NTOS_OK) && (sem.count == 0U));

    /* count đã về 0 và timeout là NTOS_NO_WAIT - take() phải trả lỗi ngay chứ
     * không được rơi vào nhánh block dù chỉ trong một khoảnh khắc, vì
     * NO_WAIT nghĩa là caller không chấp nhận bị treo. */
    status = ntos_sem_take(&sem, NTOS_NO_WAIT);
    uart_write_pass_fail("no_wait_returns_timeout_without_blocking",
        (status == NTOS_ERR_TIMEOUT) && (sem.count == 0U));

    /* Vì conductor luôn priority cao hơn cả hai waiter, chúng chỉ có cơ hội
     * chạy khi conductor tự nhường CPU như thế này. Cả hai gọi
     * ntos_sem_take(WAIT_FOREVER) ngay khi bắt đầu chạy, và count vẫn đang là
     * 0, nên khi conductor tỉnh lại, cả hai phải đã rơi vào trạng thái chờ. */
    ntos_delay(SETTLE_TICKS);

    uart_write_pass_fail("both_waiters_blocked",
        waiter_blocked_correctly(&task_waiter_high_tcb) &&
        waiter_blocked_correctly(&task_waiter_low_tcb) &&
        (sem.waiting_tasks.length == 2U));

    /* waiter_high vào wait list sau waiter_low (thứ tự tạo task) nhưng có
     * priority cao hơn - give() phải chọn nó trước để chứng minh việc chọn
     * waiter dựa trên priority chứ không phải thứ tự đăng ký. Token được
     * trao thẳng cho waiter_high nên count phải giữ nguyên 0: nếu give() vừa
     * trao token cho waiter vừa tăng count, cùng một lần give sẽ bị tính hai
     * lần - một lần nằm trong count, một lần đã giao cho waiter_high. */
    status = ntos_sem_give(&sem);
    uart_write_pass_fail("give_wakes_high_priority_waiter_first_direct_handoff",
        (status == NTOS_OK) && (sem.count == 0U) &&
        (task_waiter_high_tcb.state == NTOS_TASK_READY) &&
        (task_waiter_high_tcb.ready_item.container == &ready_lists[HIGH_PRIORITY]) &&
        waiter_blocked_correctly(&task_waiter_low_tcb));

    /* waiter_low là waiter duy nhất còn lại nên lần give() thứ hai phải chọn
     * đúng nó, không còn ai để so priority nữa. */
    status = ntos_sem_give(&sem);
    uart_write_pass_fail("give_wakes_low_priority_waiter",
        (status == NTOS_OK) && (sem.count == 0U) &&
        (task_waiter_low_tcb.state == NTOS_TASK_READY) &&
        ntos_list_is_empty(&sem.waiting_tasks));

    /* Hai give() ở trên chỉ chuyển waiter sang READY, chưa chắc chúng đã
     * thực sự chạy lại (conductor vẫn priority cao hơn). Nhường CPU ở đây để
     * cả hai có cơ hội resume và tự ghi lại status trả về từ chính lời gọi
     * take() của chúng - chỉ kiểm state == READY không chứng minh được
     * take() có thực sự trả NTOS_OK hay không (ví dụ nếu logic wake sai và
     * task quay lại nhánh lỗi thay vì trả thẳng OK). */
    ntos_delay(SETTLE_TICKS);

    uart_write_pass_fail("both_waiters_resumed_take_returned_ok",
        (g_waiter_high_take_status == NTOS_OK) && (g_waiter_low_take_status == NTOS_OK));

    /* Wait list hiện đang rỗng nên give() không còn ai để trao token - phải
     * tăng count thay vì lặng lẽ bỏ qua sự kiện give này. */
    status = ntos_sem_give(&sem);
    uart_write_pass_fail("give_without_waiter_increments_count", (status == NTOS_OK) && (sem.count == 1U));

    /* count vừa chạm max_count ở bước trên - give() tiếp theo phải từ chối
     * thay vì để count vượt giới hạn đã khai báo lúc init. */
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
