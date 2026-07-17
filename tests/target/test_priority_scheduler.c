/*
 * CHANG 6 - Buoc 6B: kiem tra fixed-priority scheduler O(n).
 *
 * 3 task CPU-bound, khong goi ntos_yield()/ntos_delay():
 *   - led0_task  (priority cao)  : toggle LED0.
 *   - uart_task  (priority cao)  : gui counter qua UART, cung in ket qua
 *                                  starvation cua led1_task.
 *   - led1_task  (priority thap) : toggle LED1.
 *
 * led0_task va uart_task cung priority cao nen round-robin voi nhau; led1_task
 * priority thap hon nen KHONG BAO GIO duoc chon khi con task priority cao dang
 * READY - LED1 se dung yen vinh vien (starvation hop le, khong phai bug). Neu
 * scheduler con la round-robin phang (chua ap dung priority), LED1 se toggle
 * deu nhu LED0 va dong "led1_starved" se bao FAIL.
 */
#include <ntos/ntos.h>
#include <board.h>
#include <uart.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define STACK_WORDS ((size_t)64U)
#define HIGH_PRIORITY ((ntos_priority_t)5U)
#define LOW_PRIORITY ((ntos_priority_t)1U)
#define LED_BUSY_WAIT_LOOPS 300000U
#define UART_BUSY_WAIT_LOOPS 1500000U

static ntos_tcb_t led0_task_tcb;
static ntos_tcb_t uart_task_tcb;
static ntos_tcb_t led1_task_tcb;
static _Alignas(8) uint32_t led0_task_stack[STACK_WORDS];
static _Alignas(8) uint32_t uart_task_stack[STACK_WORDS];
static _Alignas(8) uint32_t led1_task_stack[STACK_WORDS];

/* Debugger/UART doc cac bien nay de kiem chung led1_task bi starve. */
static volatile uint32_t g_led0_toggle_count;
static volatile uint32_t g_uart_send_count;
static volatile uint32_t g_led1_toggle_count;

static void busy_wait(volatile uint32_t count)
{
    while (count-- > 0U)
    {
        __asm volatile("nop");
    }
}

/* TEST-ONLY: doi so nguyen khong dau sang chuoi thap phan, khong dung stdio. */
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

/* Priority cao - round-robin voi uart_task. */
static void led0_task(void *argument)
{
    (void)argument;

    for (;;)
    {
        board_led_toggle(BOARD_LED_D0);
        g_led0_toggle_count++;
        busy_wait(LED_BUSY_WAIT_LOOPS);
    }
}

/* Priority cao - round-robin voi led0_task; dong thoi bao cao PASS/FAIL cua
 * starvation qua UART. */
static void uart_task(void *argument)
{
    (void)argument;

    for (;;)
    {
        bool led1_starved = (g_led1_toggle_count == 0U);

        bsp_uart_write_string("Task counter: ");
        uart_write_uint32(g_uart_send_count);
        bsp_uart_write_string("\r\n[6B] led1_starved_while_high_priority_ready: ");
        bsp_uart_write_string(led1_starved ? "PASS\r\n\r\n" : "FAIL\r\n\r\n");

        g_uart_send_count++;
        busy_wait(UART_BUSY_WAIT_LOOPS);
    }
}

/* Priority thap - khong bao gio duoc scheduler chon khi led0_task/uart_task
 * van con READY, nen LED1 se dung yen vinh vien. */
static void led1_task(void *argument)
{
    (void)argument;

    for (;;)
    {
        board_led_toggle(BOARD_LED_D1);
        g_led1_toggle_count++;
        busy_wait(LED_BUSY_WAIT_LOOPS);
    }
}

int main(void)
{
    ntos_status_t status_led0;
    ntos_status_t status_uart;
    ntos_status_t status_led1;

    /* board_init() da lam ca GPIO LED lan bsp_uart_init() - khong goi lai o day. */
    board_init();
    ntos_init();

    status_led0 = ntos_task_create_static(&led0_task_tcb, "led0_task", led0_task, NULL,
        led0_task_stack, STACK_WORDS, HIGH_PRIORITY);
    status_uart = ntos_task_create_static(&uart_task_tcb, "uart_task", uart_task, NULL,
        uart_task_stack, STACK_WORDS, HIGH_PRIORITY);
    status_led1 = ntos_task_create_static(&led1_task_tcb, "led1_task", led1_task, NULL,
        led1_task_stack, STACK_WORDS, LOW_PRIORITY);

    if ((status_led0 != NTOS_OK) || (status_uart != NTOS_OK) || (status_led1 != NTOS_OK))
    {
        for (;;)
        {
        }
    }

    ntos_start(&led0_task_tcb);

    for (;;)
    {
    }
}
