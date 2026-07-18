/*
 * Integration test cho Chặng 9A - semaphore give TỪ ISR (KEY2/EXTI1). Xem
 * chang09_semaphore.c cho phần core (task context, không ISR).
 *
 * Kịch bản:
 *   task_high (priority cao) block WAIT_FOREVER trên sem ngay từ đầu (count
 *   khởi tạo == 0).
 *   task_low (priority thấp) busy-loop CPU-bound liên tục - đóng vai "đang
 *   chạy" để chứng minh preemption thật khi nút được bấm.
 *   Nút KEY2 (PC1, EXTI1 - vector riêng, không dùng chung như EXTI9_5/15_10)
 *   sinh ngắt -> EXTI1_IRQHandler chỉ clear pending rồi gọi
 *   ntos_sem_give_from_isr() - không yield, không tự context switch, ISR
 *   ngắn đúng convention.
 *   ntos_sem_give_from_isr() tự set higher_priority_task_woken và tự request
 *   PendSV nếu cần - task_high phải preempt task_low gần như ngay lập tức
 *   sau khi ISR return.
 */
#include <ntos/ntos.h>
#include <board.h>
#include <exti.h>
#include <uart.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define STACK_WORDS ((size_t)64U)
#define HIGH_PRIORITY ((ntos_priority_t)5U)
#define LOW_PRIORITY ((ntos_priority_t)1U)

static ntos_tcb_t task_high_tcb;
static ntos_tcb_t task_low_tcb;
static _Alignas(8) uint32_t task_high_stack[STACK_WORDS];
static _Alignas(8) uint32_t task_low_stack[STACK_WORDS];

static ntos_semaphore_t sem;

static volatile uint32_t g_sem_isr_wake_count;
static volatile uint32_t g_low_run_count;
static volatile bool g_higher_priority_task_woken;

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

static void task_high(void *argument)
{
    (void)argument;

    for (;;)
    {
        ntos_status_t status = ntos_sem_take(&sem, NTOS_WAIT_FOREVER);

        g_sem_isr_wake_count++;
        board_led_toggle(BOARD_LED_D0);

        bsp_uart_write_string("[9A ISR] semaphore wake #");
        uart_write_uint32(g_sem_isr_wake_count);
        bsp_uart_write_string(" take_status_ok=");
        bsp_uart_write_string((status == NTOS_OK) ? "true" : "false");
        bsp_uart_write_string(" higher_priority_task_woken=");
        bsp_uart_write_string(g_higher_priority_task_woken ? "true" : "false");
        bsp_uart_write_string("\r\n");
    }
}

/* CPU-bound cố ý không delay - đóng vai "đang chạy" để chứng minh task_high
 * (priority cao hơn) preempt được nó ngay khi ISR give bắn ra. */
static void task_low(void *argument)
{
    (void)argument;

    /* Enable NVIC EXTI1 ngay khi task này bắt đầu chạy - không bắt buộc về an
     * toàn (ntos_sem_give_from_isr() an toàn kể cả trước khi scheduler chạy,
     * xem @note của nó trong ntos_sem.h), chỉ là good practice: tránh một lần
     * bấm sớm bị "nuốt" thành count tăng lúc chưa ai chờ. */
    bsp_exti_config(BOARD_KEY2_PORT, BOARD_KEY2_PIN, BSP_EXTI_TRIGGER_FALLING);

    for (;;)
    {
        g_low_run_count++;
    }
}

void EXTI1_IRQHandler(void)
{
    bool higher_priority_task_woken = false;

    bsp_exti_clear_pending(BOARD_KEY2_PIN);

    (void)ntos_sem_give_from_isr(&sem, &higher_priority_task_woken);
    g_higher_priority_task_woken = higher_priority_task_woken;
    /* ntos_sem_give_from_isr() đã tự request PendSV nếu cần - không làm gì thêm. */
}

int main(void)
{
    ntos_status_t status_high;
    ntos_status_t status_low;

    board_init();
    ntos_init();

    ntos_sem_init(&sem, 0U, 1U);

    status_high = ntos_task_create_static(&task_high_tcb, "high", task_high,
        NULL, task_high_stack, STACK_WORDS, HIGH_PRIORITY);
    status_low = ntos_task_create_static(&task_low_tcb, "low", task_low,
        NULL, task_low_stack, STACK_WORDS, LOW_PRIORITY);

    if ((status_high != NTOS_OK) || (status_low != NTOS_OK))
    {
        for (;;)
        {
        }
    }

    ntos_start(&task_high_tcb);

    for (;;)
    {
    }
}
