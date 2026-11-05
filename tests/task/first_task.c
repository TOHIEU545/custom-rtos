#include <rtos.h>
#include <bsp.h>
#include <stddef.h>
#include <stdint.h>

#define STACK_WORDS ((size_t)64U)
#define TASK_ARGUMENT ((void *)0x12345678U)

#define BLINK_DELAY_LOOPS 100U

static rtos_tcb_t first_task_tcb;
static _Alignas(8) uint32_t first_task_stack[STACK_WORDS];

/* Debugger đọc các biến này sau khi task chạy để kiểm chứng flow. */
static volatile uint32_t g_argument;
static volatile uint32_t g_psp;
static volatile uint32_t g_msp;
static volatile uint32_t g_control;
static volatile uint32_t g_ipsr;
static volatile uint32_t *g_local_addr;

static void delay_busy(uint32_t loops)
{
    volatile uint32_t i, j;
    for (i = 0; i < loops; i++)
    {
        for (j = 0; j < 0x2aff; j++)
        {

        }
    }
}

static void first_task(void *argument)
{
    uint32_t local_marker = 0xC0FFEEU;

    g_argument = (uint32_t)(uintptr_t)argument;
    g_local_addr = &local_marker;

    __asm volatile("mrs %0, psp" : "=r"(g_psp));
    __asm volatile("mrs %0, msp" : "=r"(g_msp));
    __asm volatile("mrs %0, control" : "=r"(g_control));
    __asm volatile("mrs %0, ipsr" : "=r"(g_ipsr));

    board_init();

    for (;;)
    {
        board_led_toggle(BOARD_LED_D0);
        delay_busy(BLINK_DELAY_LOOPS);
    }
}

int main(void)
{
    rtos_status_t status;

    /* rtos_init() phải gọi trước khi tạo task bất kỳ nào - nó init
     * ready_lists[] mà rtos_task_create_static() sẽ đưa task mới vào ngay. */
    rtos_init();

    status = rtos_task_create_static(&first_task_tcb, "first_task", first_task, TASK_ARGUMENT,
                                first_task_stack, STACK_WORDS, 0U);

    if (status != RTOS_OK)
    {
        for (;;)
        {
        }
    }

    rtos_start(&first_task_tcb, board_cpu_clock_hz());

    for (;;)
    {
    }
}