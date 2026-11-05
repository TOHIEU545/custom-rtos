/*
 * Queue FIFO contract - single task, no blocking involved.
 *
 * Verifies ring-buffer FIFO order, full/empty handling, and index
 * wrap-around across multiple cycles, all through RTOS_NO_WAIT calls.
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

static rtos_queue_t queue_a;
static uint32_t queue_a_storage[2];

static void uart_write_pass_fail(const char *name, bool ok)
{
    bsp_uart_write_string("[queue] ");
    bsp_uart_write_string(name);
    bsp_uart_write_string(ok ? ": PASS\r\n" : ": FAIL\r\n");
}

static void task_conductor(void *argument)
{
    rtos_status_t status;
    uint32_t value;

    (void)argument;

    bsp_uart_write_string("[queue] test_queue_fifo: bat dau\r\n");

    value = 10U;
    status = rtos_queue_send(&queue_a, &value, RTOS_NO_WAIT);
    uart_write_pass_fail("send_first_item_ok", (status == RTOS_OK) && (queue_a.count == 1U));

    value = 20U;
    status = rtos_queue_send(&queue_a, &value, RTOS_NO_WAIT);
    uart_write_pass_fail("send_second_item_fills_queue_and_wraps_write_index",
        (status == RTOS_OK) && (queue_a.count == 2U) && (queue_a.write_index == 0U));

    value = 30U;
    status = rtos_queue_send(&queue_a, &value, RTOS_NO_WAIT);
    uart_write_pass_fail("send_no_wait_on_full_returns_queue_full",
        (status == RTOS_ERR_QUEUE_FULL) && (queue_a.count == 2U));

    value = 0U;
    status = rtos_queue_receive(&queue_a, &value, RTOS_NO_WAIT);
    uart_write_pass_fail("receive_returns_fifo_first_item",
        (status == RTOS_OK) && (value == 10U) && (queue_a.count == 1U));

    status = rtos_queue_receive(&queue_a, &value, RTOS_NO_WAIT);
    uart_write_pass_fail("receive_returns_fifo_second_item_and_wraps_read_index",
        (status == RTOS_OK) && (value == 20U) && (queue_a.count == 0U) && (queue_a.read_index == 0U));

    status = rtos_queue_receive(&queue_a, &value, RTOS_NO_WAIT);
    uart_write_pass_fail("receive_no_wait_on_empty_returns_queue_empty",
        status == RTOS_ERR_QUEUE_EMPTY);

    /* Gửi thêm sau khi cả hai index đã wrap về 0 một lần - kiểm wrap ổn định
     * qua nhiều vòng, không chỉ đúng lần đầu. */
    value = 40U;
    (void)rtos_queue_send(&queue_a, &value, RTOS_NO_WAIT);
    value = 50U;
    status = rtos_queue_send(&queue_a, &value, RTOS_NO_WAIT);
    uart_write_pass_fail("send_after_wrap_still_ok", (status == RTOS_OK) && (queue_a.count == 2U));

    status = rtos_queue_receive(&queue_a, &value, RTOS_NO_WAIT);
    uart_write_pass_fail("receive_after_wrap_fifo_correct_first", (status == RTOS_OK) && (value == 40U));

    status = rtos_queue_receive(&queue_a, &value, RTOS_NO_WAIT);
    uart_write_pass_fail("receive_after_wrap_fifo_correct_second", (status == RTOS_OK) && (value == 50U));

    bsp_uart_write_string("[queue] test_queue_fifo: xong\r\n");

    for (;;)
    {
        rtos_delay(HEARTBEAT_TICKS);
    }
}

int main(void)
{
    rtos_status_t status_queue_a;
    rtos_status_t status_conductor;

    board_init();
    rtos_init();

    status_queue_a = rtos_queue_init_static(&queue_a, queue_a_storage, sizeof(uint32_t), 2U);

    if (status_queue_a != RTOS_OK)
    {
        for (;;)
        {
        }
    }

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
