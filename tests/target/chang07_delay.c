/*
 * CHANG 7 - Demo ntos_delay(): 3 task, khong task nao busy-wait de tao nhip.
 *
 *   task_uart (priority cao nhat) -> in counter qua UART.
 *   task_led0 (priority thap hon) -> toggle LED0.
 *   task_led1 (priority thap hon, bang task_led0)  -> toggle LED1.
 *
 * Ca 3 deu goi ntos_delay(...) thay vi busy_wait() nhu Chang 5/6. task_led0 va
 * task_led1 cung priority nen round-robin voi nhau moi khi ca 2 cung READY;
 * task_uart priority cao hon nen luon duoc chon truoc neu no cung READY tai
 * thoi diem do - nhung vi ca 3 phan lon thoi gian deu dang BLOCKED (delay),
 * phan lon thoi gian idle (priority 0) moi la task duy nhat con READY.
 */
#include <ntos/ntos.h>
#include <board.h>
#include <uart.h>
#include <stddef.h>
#include <stdint.h>

#define STACK_WORDS ((size_t)64U)
#define UART_PRIORITY ((ntos_priority_t)2U)
#define LED_PRIORITY ((ntos_priority_t)1U)
#define UART_DELAY_TICKS 500U
#define LED0_DELAY_TICKS 100U
#define LED1_DELAY_TICKS 250U

static ntos_tcb_t task_uart_tcb;
static ntos_tcb_t task_led0_tcb;
static ntos_tcb_t task_led1_tcb;
static _Alignas(8) uint32_t task_uart_stack[STACK_WORDS];
static _Alignas(8) uint32_t task_led0_stack[STACK_WORDS];
static _Alignas(8) uint32_t task_led1_stack[STACK_WORDS];

/* Debugger doc cac bien nay de kiem chung ca 3 task deu duoc wake dung nhip. */
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
        ntos_delay(UART_DELAY_TICKS);

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
        ntos_delay(LED0_DELAY_TICKS);
        board_led_toggle(BOARD_LED_D0);
        g_led0_toggle_count++;
    }
}

static void task_led1(void *argument)
{
    (void)argument;

    for (;;)
    {
        ntos_delay(LED1_DELAY_TICKS);
        board_led_toggle(BOARD_LED_D1);
        g_led1_toggle_count++;
    }
}

int main(void)
{
    ntos_status_t status_uart;
    ntos_status_t status_led0;
    ntos_status_t status_led1;

    /* board_init() da lam ca GPIO LED lan bsp_uart_init() - khong goi lai o day. */
    board_init();
    ntos_init();

    status_uart = ntos_task_create_static(&task_uart_tcb, "task_uart", task_uart, NULL,
        task_uart_stack, STACK_WORDS, UART_PRIORITY);
    status_led0 = ntos_task_create_static(&task_led0_tcb, "task_led0", task_led0, NULL,
        task_led0_stack, STACK_WORDS, LED_PRIORITY);
    status_led1 = ntos_task_create_static(&task_led1_tcb, "task_led1", task_led1, NULL,
        task_led1_stack, STACK_WORDS, LED_PRIORITY);

    if ((status_uart != NTOS_OK) || (status_led0 != NTOS_OK) || (status_led1 != NTOS_OK))
    {
        for (;;)
        {
        }
    }

    ntos_start(&task_uart_tcb);

    for (;;)
    {
    }
}
