/*
 * Demo preemptive round-robin bằng SysTick: 3 task CPU-bound song song,
 * KHÔNG task nào gọi ntos_yield()/ntos_delay(). Nếu SysTick preemption
 * không hoạt động, task đầu tiên được chọn (led1_task) sẽ chiếm CPU vĩnh
 * viễn và LED2/UART sẽ không bao giờ chạy - đó là bằng chứng trực quan.
 */
#include <ntos/ntos.h>
#include <board.h>
#include <uart.h>
#include <stddef.h>
#include <stdint.h>

#define STACK_WORDS ((size_t)64U)
#define LED_BUSY_WAIT_LOOPS 300000U
#define UART_BUSY_WAIT_LOOPS 1500000U

static ntos_tcb_t led1_task_tcb;
static ntos_tcb_t led2_task_tcb;
static ntos_tcb_t uart_task_tcb;
static _Alignas(8) uint32_t led1_task_stack[STACK_WORDS];
static _Alignas(8) uint32_t led2_task_stack[STACK_WORDS];
static _Alignas(8) uint32_t uart_task_stack[STACK_WORDS];

/* Debugger đọc các biến này để kiểm chứng cả 3 task đều chạy dù không task nào yield. */
static volatile uint32_t g_led1_toggle_count;
static volatile uint32_t g_led2_toggle_count;
static volatile uint32_t g_uart_send_count;

/* TEST-ONLY: không dùng SysTick/kernel tick để tạo delay - chỉ là busy loop
 * làm chậm demo cho mắt người quan sát được LED/UART. */
static void busy_wait(volatile uint32_t count)
{
    while (count-- > 0U)
    {
        __asm volatile("nop");
    }
}

/* TEST-ONLY: đổi số nguyên không dấu sang chuỗi thập phân, không dùng stdio
 * (project không dùng heap/printf ở bất kỳ nơi nào khác). */
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

static void led1_task(void *argument)
{
    (void)argument;

    board_led_set(BOARD_LED_D0, true);

    for (;;)
    {
        board_led_toggle(BOARD_LED_D0);
        g_led1_toggle_count++;
        busy_wait(LED_BUSY_WAIT_LOOPS);
    }
}

static void led2_task(void *argument)
{
    (void)argument;

    board_led_set(BOARD_LED_D1, false);

    for (;;)
    {
        board_led_toggle(BOARD_LED_D1);
        g_led2_toggle_count++;
        busy_wait(LED_BUSY_WAIT_LOOPS);
    }
}

static void uart_task(void *argument)
{
    (void)argument;

    for (;;)
    {
        bsp_uart_write_string("Task 3 counter: ");
        uart_write_uint32(g_uart_send_count);
        bsp_uart_write_string("\r\n");

        g_uart_send_count++;
        busy_wait(UART_BUSY_WAIT_LOOPS);
    }
}

int main(void)
{
    ntos_status_t status_led1;
    ntos_status_t status_led2;
    ntos_status_t status_uart;

    /* board_init() đã làm cả GPIO LED lẫn bsp_uart_init() - không gọi lại ở đây. */
    board_init();

    ntos_init();

    status_led1 = ntos_task_create_static(&led1_task_tcb, "led1_task", led1_task, NULL,
        led1_task_stack, STACK_WORDS, 0U);
    status_led2 = ntos_task_create_static(&led2_task_tcb, "led2_task", led2_task, NULL,
        led2_task_stack, STACK_WORDS, 0U);
    status_uart = ntos_task_create_static(&uart_task_tcb, "uart_task", uart_task, NULL,
        uart_task_stack, STACK_WORDS, 0U);

    if ((status_led1 != NTOS_OK) || (status_led2 != NTOS_OK) || (status_uart != NTOS_OK))
    {
        for (;;)
        {
        }
    }

    ntos_start(&led1_task_tcb);

    for (;;)
    {
    }
}
