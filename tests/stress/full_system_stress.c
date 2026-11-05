#include <rtos.h>

#include <bsp.h>

#include <stddef.h>
#include <stdint.h>

#define STACK_WORDS ((size_t)96U)

#define REPORTER_PRIORITY ((rtos_priority_t)7U)
#define SEM_TAKER_PRIORITY ((rtos_priority_t)6U)
#define SEM_GIVER_PRIORITY ((rtos_priority_t)5U)
#define QUEUE_SENDER_PRIORITY ((rtos_priority_t)4U)
#define QUEUE_RECEIVER_PRIORITY ((rtos_priority_t)3U)
#define PERIODIC_PRIORITY ((rtos_priority_t)2U)
#define YIELD_PRIORITY ((rtos_priority_t)1U)

#define QUEUE_CAPACITY ((size_t)4U)

#define SEM_PERIOD_TICKS ((rtos_tick_t)3U)
#define QUEUE_PAUSE_TICKS ((rtos_tick_t)2U)
#define PERIODIC_TICKS ((rtos_tick_t)7U)
#define REPORT_PERIOD_TICKS ((rtos_tick_t)2000U)

#define QUEUE_BURST_SIZE ((uint32_t)32U)

static rtos_tcb_t reporter_tcb;
static rtos_tcb_t sem_giver_tcb;
static rtos_tcb_t sem_taker_tcb;
static rtos_tcb_t queue_sender_tcb;
static rtos_tcb_t queue_receiver_tcb;
static rtos_tcb_t periodic_tcb;
static rtos_tcb_t yield_a_tcb;
static rtos_tcb_t yield_b_tcb;

static _Alignas(8) uint32_t reporter_stack[STACK_WORDS];
static _Alignas(8) uint32_t sem_giver_stack[STACK_WORDS];
static _Alignas(8) uint32_t sem_taker_stack[STACK_WORDS];
static _Alignas(8) uint32_t queue_sender_stack[STACK_WORDS];
static _Alignas(8) uint32_t queue_receiver_stack[STACK_WORDS];
static _Alignas(8) uint32_t periodic_stack[STACK_WORDS];
static _Alignas(8) uint32_t yield_a_stack[STACK_WORDS];
static _Alignas(8) uint32_t yield_b_stack[STACK_WORDS];

static rtos_semaphore_t stress_sem;

static rtos_queue_t stress_queue;
static uint32_t queue_storage[QUEUE_CAPACITY];

/* Scheduler stress counters. */
static volatile uint32_t g_yield_a;
static volatile uint32_t g_yield_b;
static volatile uint32_t g_periodic;

/* Semaphore stress counters. */
static volatile uint32_t g_sem_give;
static volatile uint32_t g_sem_take;

/* Queue stress counters. */
static volatile uint32_t g_queue_send;
static volatile uint32_t g_queue_receive;
static volatile uint32_t g_queue_error;

/* Reporter diagnostics. */
static volatile uint32_t g_report_count;
static volatile uint32_t g_stall_error;

/* --------------------------------------------------------- */

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

static void print_value(const char *name, uint32_t value)
{
    bsp_uart_write_string(name);
    uart_write_uint32(value);
}

/* --------------------------------------------------------- */
/* Continuously force round-robin context switches. */

static void yield_a_task(void *argument)
{
    (void)argument;

    for (;;)
    {
        g_yield_a++;
        rtos_yield();
    }
}

static void yield_b_task(void *argument)
{
    (void)argument;

    for (;;)
    {
        g_yield_b++;
        rtos_yield();
    }
}

/* --------------------------------------------------------- */
/* Independent SysTick delay/wake workload. */

static void periodic_task(void *argument)
{
    (void)argument;

    for (;;)
    {
        rtos_delay(PERIODIC_TICKS);
        g_periodic++;
    }
}

/* --------------------------------------------------------- */
/* Semaphore producer. */

static void sem_giver_task(void *argument)
{
    (void)argument;

    for (;;)
    {
        rtos_delay(SEM_PERIOD_TICKS);

        if (rtos_sem_give(&stress_sem) == RTOS_OK)
        {
            g_sem_give++;
        }
    }
}

/*
 * Higher priority than the giver.
 *
 * A give wakes this task and causes priority preemption.
 */
static void sem_taker_task(void *argument)
{
    (void)argument;

    for (;;)
    {
        if (rtos_sem_take(&stress_sem, RTOS_WAIT_FOREVER) == RTOS_OK)
        {
            g_sem_take++;
        }
    }
}

/* --------------------------------------------------------- */
/*
 * Sender has higher priority than receiver.
 *
 * It therefore fills the small queue rapidly. Once the queue is full,
 * the sender blocks and the receiver can run. Receiving an item wakes
 * the sender again.
 *
 * This repeatedly exercises:
 *
 * sender -> queue full -> BLOCK
 * receiver -> receive -> wake sender
 * sender -> resume -> re-check -> send
 */
static void queue_sender_task(void *argument)
{
    uint32_t value = 0U;
    uint32_t burst_count = 0U;

    (void)argument;

    for (;;)
    {
        if (rtos_queue_send(&stress_queue, &value, RTOS_WAIT_FOREVER) == RTOS_OK)
        {
            g_queue_send++;

            value++;
            burst_count++;
        }

        /*
         * Occasionally block the producer so the receiver can completely
         * drain the queue and reach the empty-blocking path as well.
         */
        if (burst_count >= QUEUE_BURST_SIZE)
        {
            burst_count = 0U;
            rtos_delay(QUEUE_PAUSE_TICKS);
        }
    }
}

/*
 * Every queue item is a monotonically increasing sequence number.
 *
 * Any lost, duplicated, or incorrectly ordered item increments
 * g_queue_error.
 */
static void queue_receiver_task(void *argument)
{
    uint32_t expected = 0U;

    (void)argument;

    for (;;)
    {
        uint32_t value;

        if (rtos_queue_receive(&stress_queue, &value, RTOS_WAIT_FOREVER) == RTOS_OK)
        {
            g_queue_receive++;

            if (value != expected)
            {
                g_queue_error++;

                /* Resynchronize after an error. */
                expected = value;
            }

            expected++;
        }
    }
}

/* --------------------------------------------------------- */

static void print_stack(const char *name, const rtos_tcb_t *tcb)
{
    rtos_task_stack_info_t info;

    bsp_uart_write_string("  ");
    bsp_uart_write_string(name);
    bsp_uart_write_string(": ");

    if (rtos_task_stack_get_info(tcb, &info) != RTOS_OK)
    {
        bsp_uart_write_string("INFO_ERROR\r\n");
        return;
    }

    bsp_uart_write_string("used=");
    uart_write_uint32((uint32_t)info.high_water_mark_words);

    bsp_uart_write_string("/");
    uart_write_uint32((uint32_t)info.stack_words);

    bsp_uart_write_string(" free=");
    uart_write_uint32((uint32_t)info.untouched_words);

    bsp_uart_write_string(" guard=");
    bsp_uart_write_string(info.guard_intact ? "OK\r\n" : "CORRUPTED\r\n");
}

/* --------------------------------------------------------- */
/*
 * Only this task writes UART.
 *
 * It also checks whether important counters are still progressing between
 * reports. A stopped counter can indicate starvation, deadlock, or a task
 * that was incorrectly left blocked.
 */
static void reporter_task(void *argument)
{
    uint32_t previous_sem_take = 0U;
    uint32_t previous_queue_receive = 0U;
    uint32_t previous_periodic = 0U;
    uint32_t previous_yield_a = 0U;
    uint32_t previous_yield_b = 0U;

    (void)argument;

    bsp_uart_write_string("\r\n"
                          "========================================\r\n"
                          " MyRTOS FULL SYSTEM STRESS TEST\r\n"
                          "========================================\r\n");

    for (;;)
    {
        rtos_delay(REPORT_PERIOD_TICKS);

        g_report_count++;

        /*
         * Every subsystem should have made progress since the previous report.
         */
        if ((g_sem_take == previous_sem_take) || (g_queue_receive == previous_queue_receive) ||
            (g_periodic == previous_periodic) || (g_yield_a == previous_yield_a) ||
            (g_yield_b == previous_yield_b))
        {
            g_stall_error++;
        }

        previous_sem_take = g_sem_take;
        previous_queue_receive = g_queue_receive;
        previous_periodic = g_periodic;
        previous_yield_a = g_yield_a;
        previous_yield_b = g_yield_b;

        bsp_uart_write_string("\r\n---------- REPORT ----------\r\n");

        print_value("report=", g_report_count);
        print_value(" tick=", rtos_tick_get());
        bsp_uart_write_string("\r\n");

        print_value("yield_a=", g_yield_a);
        print_value(" yield_b=", g_yield_b);
        bsp_uart_write_string("\r\n");

        print_value("periodic=", g_periodic);
        bsp_uart_write_string("\r\n");

        print_value("sem_give=", g_sem_give);
        print_value(" sem_take=", g_sem_take);
        bsp_uart_write_string("\r\n");

        print_value("queue_send=", g_queue_send);
        print_value(" queue_receive=", g_queue_receive);
        bsp_uart_write_string("\r\n");

        print_value("queue_errors=", g_queue_error);
        print_value(" stall_errors=", g_stall_error);
        bsp_uart_write_string("\r\n");

        bsp_uart_write_string("stack:\r\n");

        print_stack("reporter", &reporter_tcb);
        print_stack("sem_giver", &sem_giver_tcb);
        print_stack("sem_taker", &sem_taker_tcb);
        print_stack("queue_sender", &queue_sender_tcb);
        print_stack("queue_receiver", &queue_receiver_tcb);
        print_stack("periodic", &periodic_tcb);
        print_stack("yield_a", &yield_a_tcb);
        print_stack("yield_b", &yield_b_tcb);

        bsp_uart_write_string("----------------------------\r\n");
    }
}

/* --------------------------------------------------------- */

static bool create_tasks(void)
{
    if (rtos_task_create_static(&reporter_tcb, "reporter", reporter_task, NULL, reporter_stack,
                                STACK_WORDS, REPORTER_PRIORITY) != RTOS_OK)
    {
        return false;
    }

    if (rtos_task_create_static(&sem_giver_tcb, "sem_giver", sem_giver_task, NULL, sem_giver_stack,
                                STACK_WORDS, SEM_GIVER_PRIORITY) != RTOS_OK)
    {
        return false;
    }

    if (rtos_task_create_static(&sem_taker_tcb, "sem_taker", sem_taker_task, NULL, sem_taker_stack,
                                STACK_WORDS, SEM_TAKER_PRIORITY) != RTOS_OK)
    {
        return false;
    }

    if (rtos_task_create_static(&queue_sender_tcb, "queue_sender", queue_sender_task, NULL,
                                queue_sender_stack, STACK_WORDS, QUEUE_SENDER_PRIORITY) != RTOS_OK)
    {
        return false;
    }

    if (rtos_task_create_static(&queue_receiver_tcb, "queue_receiver", queue_receiver_task, NULL,
                                queue_receiver_stack, STACK_WORDS,
                                QUEUE_RECEIVER_PRIORITY) != RTOS_OK)
    {
        return false;
    }

    if (rtos_task_create_static(&periodic_tcb, "periodic", periodic_task, NULL, periodic_stack,
                                STACK_WORDS, PERIODIC_PRIORITY) != RTOS_OK)
    {
        return false;
    }

    if (rtos_task_create_static(&yield_a_tcb, "yield_a", yield_a_task, NULL, yield_a_stack,
                                STACK_WORDS, YIELD_PRIORITY) != RTOS_OK)
    {
        return false;
    }

    if (rtos_task_create_static(&yield_b_tcb, "yield_b", yield_b_task, NULL, yield_b_stack,
                                STACK_WORDS, YIELD_PRIORITY) != RTOS_OK)
    {
        return false;
    }

    return true;
}

/* --------------------------------------------------------- */

int main(void)
{
    board_init();
    rtos_init();

    rtos_sem_init(&stress_sem, 0U, 1U);

    if (rtos_queue_init_static(&stress_queue, queue_storage, sizeof(uint32_t), QUEUE_CAPACITY) !=
        RTOS_OK)
    {
        for (;;)
        {
        }
    }

    if (!create_tasks())
    {
        for (;;)
        {
        }
    }

    rtos_start(&reporter_tcb, board_cpu_clock_hz());

    for (;;)
    {
    }
}