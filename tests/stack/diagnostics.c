#include <rtos.h>

#include <bsp.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define CONDUCTOR_STACK_WORDS ((size_t)64U)
#define WORKER_STACK_WORDS ((size_t)128U)

#define CONDUCTOR_PRIORITY ((rtos_priority_t)3U)
#define WORKER_PRIORITY ((rtos_priority_t)1U)

#define WORKER_CONSUME_WORDS ((size_t)30U)

#define SETTLE_TICKS ((rtos_tick_t)1000U)
#define HEARTBEAT_TICKS ((rtos_tick_t)2000U)

static rtos_tcb_t conductor_tcb;
static rtos_tcb_t worker_tcb;

static _Alignas(8) uint32_t conductor_stack[CONDUCTOR_STACK_WORDS];
static _Alignas(8) uint32_t worker_stack[WORKER_STACK_WORDS];

static volatile bool g_worker_finished;

static void uart_write_uint32(uint32_t value)
{
    char digits[10];
    size_t count = 0U;

    if (value == 0U)
    {
        bsp_uart_write_byte('0');
        return;
    }

    while (value > 0U)
    {
        digits[count++] = (char)('0' + (value % 10U));
        value /= 10U;
    }

    while (count > 0U)
    {
        bsp_uart_write_byte((uint8_t)digits[--count]);
    }
}

static void uart_write_value(const char *label, size_t words)
{
    bsp_uart_write_string(label);
    uart_write_uint32((uint32_t)words);
    bsp_uart_write_string(" words (");
    uart_write_uint32((uint32_t)(words * sizeof(uint32_t)));
    bsp_uart_write_string(" bytes)\r\n");
}

static void print_stack_info(const char *title, const rtos_task_stack_info_t *info)
{
    bsp_uart_write_string("\r\n=== ");
    bsp_uart_write_string(title);
    bsp_uart_write_string(" ===\r\n");

    uart_write_value("Total stack      : ", info->stack_words);
    uart_write_value("Maximum used     : ", info->high_water_mark_words);
    uart_write_value("Never used       : ", info->untouched_words);

    bsp_uart_write_string("Guard            : ");
    bsp_uart_write_string(info->guard_intact ? "intact\r\n" : "CORRUPTED\r\n");
}

static void uart_write_pass_fail(const char *name, bool ok)
{
    bsp_uart_write_string("[stack] ");
    bsp_uart_write_string(name);
    bsp_uart_write_string(ok ? ": PASS\r\n" : ": FAIL\r\n");
}

/*
 * This function deliberately uses local stack memory so the worker reaches
 * a deeper stack level.
 */
__attribute__((noinline)) static uint32_t worker_consume_stack(void)
{
    volatile uint32_t buffer[WORKER_CONSUME_WORDS];
    size_t i;

    for (i = 0U; i < WORKER_CONSUME_WORDS; ++i)
    {
        buffer[i] = (uint32_t)i;
    }

    return buffer[WORKER_CONSUME_WORDS - 1U];
}

static void worker_task(void *argument)
{
    (void)argument;

    /*
     * The worker only consumes stack.
     * It does not print anything, so UART output cannot interleave with the
     * conductor's diagnostic messages.
     */
    (void)worker_consume_stack();
    worker_stack[0] = 0xDEADBEEFU;

    g_worker_finished = true;

    for (;;)
    {
        rtos_delay(HEARTBEAT_TICKS);
    }
}

static void conductor_task(void *argument)
{
    rtos_task_stack_info_t info_before_worker;
    rtos_task_stack_info_t info_after_worker;

    (void)argument;

    /*
     * At this point the conductor is running first because it has higher
     * priority. The worker has not executed yet.
     */
    if (rtos_task_stack_get_info(&worker_tcb, &info_before_worker) == RTOS_OK)
    {
        print_stack_info("Worker before running", &info_before_worker);
    }

    bsp_uart_write_string("\r\nConductor blocks for a short time.\r\n"
                          "Worker can now run and consume stack.\r\n");

    /*
     * While the conductor is blocked, the lower-priority worker gets CPU time.
     */
    rtos_delay(SETTLE_TICKS);

    /*
     * The conductor runs again after the delay and reads the worker's stack.
     */
    if (rtos_task_stack_get_info(&worker_tcb, &info_after_worker) == RTOS_OK)
    {
        print_stack_info("Worker after consuming stack", &info_after_worker);
    }

    bsp_uart_write_string("\r\n");
    uart_write_pass_fail("worker_finished", g_worker_finished);
    uart_write_pass_fail("high_water_mark_increased",
        info_after_worker.high_water_mark_words > info_before_worker.high_water_mark_words);
    uart_write_pass_fail("untouched_words_decreased",
        info_after_worker.untouched_words < info_before_worker.untouched_words);
    uart_write_pass_fail("guard_still_intact", info_after_worker.guard_intact);

    for (;;)
    {
        rtos_delay(HEARTBEAT_TICKS);
    }
}

int main(void)
{
    rtos_task_stack_info_t info_created;

    board_init();
    rtos_init();

    if (rtos_task_create_static(&worker_tcb, "worker", worker_task, NULL, worker_stack,
                                WORKER_STACK_WORDS, WORKER_PRIORITY) != RTOS_OK)
    {
        for (;;)
        {
        }
    }

    if (rtos_task_create_static(&conductor_tcb, "conductor", conductor_task, NULL, conductor_stack,
                                CONDUCTOR_STACK_WORDS, CONDUCTOR_PRIORITY) != RTOS_OK)
    {
        for (;;)
        {
        }
    }

    bsp_uart_write_string("\r\n=== MyRTOS stack diagnostics ===\r\n");

    /*
     * Read the worker stack before the scheduler starts.
     * Only the initial context frame should have consumed stack at this point.
     */
    if (rtos_task_stack_get_info(&worker_tcb, &info_created) == RTOS_OK)
    {
        print_stack_info("Worker immediately after creation", &info_created);
    }

    rtos_start(&conductor_tcb, board_cpu_clock_hz());

    for (;;)
    {
    }
}