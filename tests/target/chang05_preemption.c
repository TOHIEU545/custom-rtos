/*
 * CHANG 5 - Demo preemptive round-robin bang SysTick: 3 task CPU-bound song
 * song, KHONG task nao goi ntos_yield()/ntos_delay(). Neu SysTick preemption
 * khong hoat dong, task dau tien duoc chon (led1_task) se chiem CPU vinh vien
 * va LED2/UART se khong bao gio chay - do la bang chung truc quan.
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

/* Debugger doc cac bien nay de kiem chung ca 3 task deu chay du khong task nao yield. */
static volatile uint32_t g_led1_toggle_count;
static volatile uint32_t g_led2_toggle_count;
static volatile uint32_t g_uart_send_count;

/* TEST-ONLY: khong dung SysTick/kernel tick de tao delay - chi la busy loop
 * lam cham demo cho mat nguoi quan sat duoc LED/UART. */
static void busy_wait(volatile uint32_t count)
{
    while (count-- > 0U)
    {
        __asm volatile("nop");
    }
}

/* TEST-ONLY: doi so nguyen khong dau sang chuoi thap phan, khong dung stdio
 * (project khong dung heap/printf o bat ky noi nao khac). */
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

    /* board_init() da lam ca GPIO LED lan bsp_uart_init() - khong goi lai o day. */
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
