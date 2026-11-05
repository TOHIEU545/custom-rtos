/*
 * Demo rtos_delay(): 3 task, không task nào busy-wait để tạo nhịp.
 *
 *   task_uart (priority cao nhất) -> in counter qua UART.
 *   task_led0 (priority thấp hơn) -> toggle LED0.
 *   task_led1 (priority thấp hơn, bằng task_led0)  -> toggle LED1.
 *
 * Cả 3 đều gọi rtos_delay(...) thay vì busy_wait(). task_led0 và task_led1
 * cùng priority nên round-robin với nhau mỗi khi cả 2 cùng READY; task_uart
 * priority cao hơn nên luôn được chọn trước nếu nó cũng READY tại thời điểm
 * đó - nhưng vì cả 3 phần lớn thời gian đều đang BLOCKED (delay), phần lớn
 * thời gian idle (priority 0) mới là task duy nhất còn READY.
 */
#include <rtos.h>
#include <bsp.h>
#include <stddef.h>
#include <stdint.h>

#define STACK_WORDS ((size_t)64U)
#define UART_PRIORITY ((rtos_priority_t)2U)
#define LED_PRIORITY ((rtos_priority_t)1U)
#define UART_DELAY_TICKS 500U
#define LED0_DELAY_TICKS 100U
#define LED1_DELAY_TICKS 250U

static rtos_tcb_t task_uart_tcb;
static rtos_tcb_t task_led0_tcb;
static rtos_tcb_t task_led1_tcb;
static _Alignas(8) uint32_t task_uart_stack[STACK_WORDS];
static _Alignas(8) uint32_t task_led0_stack[STACK_WORDS];
static _Alignas(8) uint32_t task_led1_stack[STACK_WORDS];

/* Debugger đọc các biến này để kiểm chứng cả 3 task đều được wake đúng nhịp. */
static volatile uint32_t g_uart_counter;
static volatile uint32_t g_led0_toggle_count;
static volatile uint32_t g_led1_toggle_count;

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

static void task_uart(void *argument)
{
    (void)argument;

    for (;;)
    {
        rtos_delay(UART_DELAY_TICKS);

        g_uart_counter++;
        bsp_uart_write_string("[7] uart counter: ");
        uart_write_uint32(g_uart_counter);
        bsp_uart_write_string(" (led0=");
        uart_write_uint32(g_led0_toggle_count);
        bsp_uart_write_string(", led1=");
        uart_write_uint32(g_led1_toggle_count);
        bsp_uart_write_string(")\r\n");
    }
}

static void task_led0(void *argument)
{
    (void)argument;

    for (;;)
    {
        rtos_delay(LED0_DELAY_TICKS);
        board_led_toggle(BOARD_LED_D0);
        g_led0_toggle_count++;
    }
}

static void task_led1(void *argument)
{
    (void)argument;

    for (;;)
    {
        rtos_delay(LED1_DELAY_TICKS);
        board_led_toggle(BOARD_LED_D1);
        g_led1_toggle_count++;
    }
}

int main(void)
{
    rtos_status_t status_uart;
    rtos_status_t status_led0;
    rtos_status_t status_led1;

    /* board_init() đã làm cả GPIO LED lẫn bsp_uart_init() - không gọi lại ở đây. */
    board_init();
    rtos_init();

    status_uart = rtos_task_create_static(&task_uart_tcb, "task_uart", task_uart, NULL,
        task_uart_stack, STACK_WORDS, UART_PRIORITY);
    status_led0 = rtos_task_create_static(&task_led0_tcb, "task_led0", task_led0, NULL,
        task_led0_stack, STACK_WORDS, LED_PRIORITY);
    status_led1 = rtos_task_create_static(&task_led1_tcb, "task_led1", task_led1, NULL,
        task_led1_stack, STACK_WORDS, LED_PRIORITY);

    if ((status_uart != RTOS_OK) || (status_led0 != RTOS_OK) || (status_led1 != RTOS_OK))
    {
        for (;;)
        {
        }
    }

    rtos_start(&task_uart_tcb, board_cpu_clock_hz());

    for (;;)
    {
    }
}
