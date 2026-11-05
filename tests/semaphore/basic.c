/*
 * Semaphore basic contract - single task, no blocking involved.
 *
 * Verifies init fields, immediate take, non-blocking timeout, give without
 * a waiter, and give rejected at max_count.
 */
#include <rtos.h>
#include <bsp.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define STACK_WORDS ((size_t)64U)
#define CONDUCTOR_PRIORITY ((rtos_priority_t)1U)
#define HEARTBEAT_TICKS ((rtos_tick_t)2000U)

static rtos_tcb_t task_conductor_tcb;
static _Alignas(8) uint32_t task_conductor_stack[STACK_WORDS];

static rtos_semaphore_t sem;

static void uart_write_pass_fail(const char *name, bool ok)
{
    bsp_uart_write_string("[semaphore] ");
    bsp_uart_write_string(name);
    bsp_uart_write_string(ok ? ": PASS\r\n" : ": FAIL\r\n");
}

static void task_conductor(void *argument)
{
    rtos_status_t status;

    (void)argument;

    bsp_uart_write_string("[semaphore] test_semaphore_basic: bat dau\r\n");

    /* count khởi tạo > 0 để bước kiểm tra kế tiếp lấy được ngay mà không phải
     * đi qua nhánh block. */
    rtos_sem_init(&sem, 1U, 1U);
    uart_write_pass_fail("init_fields_correct",
        (sem.count == 1U) && (sem.max_count == 1U) && (sem.waiting_tasks.length == 0U));

    status = rtos_sem_take(&sem, RTOS_NO_WAIT);
    uart_write_pass_fail("take_success_decrements_count", (status == RTOS_OK) && (sem.count == 0U));

    /* count đã về 0 và timeout là RTOS_NO_WAIT - take() phải trả lỗi ngay chứ
     * không được rơi vào nhánh block dù chỉ trong một khoảnh khắc. */
    status = rtos_sem_take(&sem, RTOS_NO_WAIT);
    uart_write_pass_fail("no_wait_returns_timeout_without_blocking",
        (status == RTOS_ERR_TIMEOUT) && (sem.count == 0U));

    /* Không có waiter nào từng tồn tại trong file này - give() phải tăng count
     * thay vì tìm waiter để trao token. */
    status = rtos_sem_give(&sem);
    uart_write_pass_fail("give_without_waiter_increments_count", (status == RTOS_OK) && (sem.count == 1U));

    /* count vừa chạm max_count ở bước trên - give() tiếp theo phải từ chối
     * thay vì để count vượt giới hạn đã khai báo lúc init. */
    status = rtos_sem_give(&sem);
    uart_write_pass_fail("give_at_max_count_returns_resource_full",
        (status == RTOS_ERR_RESOURCE_FULL) && (sem.count == 1U));

    bsp_uart_write_string("[semaphore] test_semaphore_basic: xong\r\n");

    for (;;)
    {
        rtos_delay(HEARTBEAT_TICKS);
    }
}

int main(void)
{
    rtos_status_t status_conductor;

    board_init();
    rtos_init();

    status_conductor = rtos_task_create_static(&task_conductor_tcb, "conductor", task_conductor,
        NULL, task_conductor_stack, STACK_WORDS, CONDUCTOR_PRIORITY);

    if (status_conductor != RTOS_OK)
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
