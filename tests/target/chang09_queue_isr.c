/*
 * Integration test cho Chặng 9B - queue send TỪ ISR (KEY1/EXTI15_10). Xem
 * chang09_queue.c cho phần core (task context, không ISR).
 *
 * Kịch bản:
 *   task_parser (priority cao) block WAIT_FOREVER trên event_queue ngay từ
 *   đầu (queue rỗng).
 *   task_low (priority thấp) busy-loop CPU-bound liên tục - đóng vai "đang
 *   chạy" để chứng minh preemption thật khi nút được bấm.
 *   Nút KEY1 (PC13, EXTI15_10 - vector DÙNG CHUNG line 10-15, khác KEY2/EXTI1
 *   dùng ở chang09_semaphore_isr.c) sinh ngắt -> phải tự kiểm
 *   bsp_exti_pending() đúng line 13 trước khi xử lý; ISR tạo uint32_t event
 *   rồi gọi ntos_queue_send_from_isr() - không yield, không tự context
 *   switch.
 *   ntos_queue_send_from_isr() tự set higher_priority_task_woken và tự
 *   request PendSV nếu cần - task_parser phải preempt task_low gần như ngay
 *   lập tức sau khi ISR return, rồi tự receive() đúng event vừa gửi.
 *   Nếu queue full, ntos_queue_send_from_isr() trả NTOS_ERR_QUEUE_FULL (ISR
 *   không block).
 */
#include <ntos/ntos.h>
#include <board.h>
#include <exti.h>
#include <uart.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define STACK_WORDS ((size_t)64U)
#define PARSER_PRIORITY ((ntos_priority_t)5U)
#define LOW_PRIORITY ((ntos_priority_t)1U)
#define EVENT_QUEUE_CAPACITY ((size_t)4U)

static ntos_tcb_t task_parser_tcb;
static ntos_tcb_t task_low_tcb;
static _Alignas(8) uint32_t task_parser_stack[STACK_WORDS];
static _Alignas(8) uint32_t task_low_stack[STACK_WORDS];

static ntos_queue_t event_queue;
static uint32_t event_queue_storage[EVENT_QUEUE_CAPACITY];

static volatile uint32_t g_button_event_counter;
static volatile uint32_t g_queue_isr_send_count;
static volatile uint32_t g_queue_isr_full_count;
static volatile uint32_t g_queue_isr_receive_count;
static volatile bool g_queue_higher_priority_task_woken;
static volatile uint32_t g_low_run_count;

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

static void task_parser(void *argument)
{
    (void)argument;

    for (;;)
    {
        uint32_t event = 0U;
        ntos_status_t status = ntos_queue_receive(&event_queue, &event, NTOS_WAIT_FOREVER);

        g_queue_isr_receive_count++;
        board_led_toggle(BOARD_LED_D1);

        bsp_uart_write_string("[9B ISR] event = ");
        uart_write_uint32(event);
        bsp_uart_write_string(" status_ok=");
        bsp_uart_write_string((status == NTOS_OK) ? "true" : "false");
        bsp_uart_write_string(" higher_priority_task_woken=");
        bsp_uart_write_string(g_queue_higher_priority_task_woken ? "true" : "false");
        bsp_uart_write_string("\r\n");
    }
}

/* CPU-bound cố ý không delay - đóng vai "đang chạy" để chứng minh task_parser
 * (priority cao hơn) preempt được nó ngay khi ISR send bắn ra. */
static void task_low(void *argument)
{
    (void)argument;

    /* Enable NVIC EXTI15_10 ngay khi task này bắt đầu chạy - không bắt buộc
     * về an toàn (ntos_queue_send_from_isr() an toàn kể cả trước khi
     * scheduler chạy, xem @note trong ntos_queue.h), chỉ là good practice. */
    bsp_exti_config(BOARD_KEY1_PORT, BOARD_KEY1_PIN, BSP_EXTI_TRIGGER_FALLING);

    for (;;)
    {
        g_low_run_count++;
    }
}

/* KEY1 = PC13 -> EXTI line 13 -> vector DÙNG CHUNG EXTI15_10 - phải tự kiểm
 * bsp_exti_pending() đúng line 13 trước khi xử lý, vì handler này còn có thể
 * bị gọi bởi line 10-12/14/15 nếu sau này có nguồn ngắt khác dùng chung. */
void EXTI15_10_IRQHandler(void)
{
    if (bsp_exti_pending(BOARD_KEY1_PIN))
    {
        bool higher_priority_task_woken = false;
        uint32_t event;
        ntos_status_t status;

        bsp_exti_clear_pending(BOARD_KEY1_PIN);

        g_button_event_counter++;
        event = g_button_event_counter;

        status = ntos_queue_send_from_isr(&event_queue, &event, &higher_priority_task_woken);
        g_queue_higher_priority_task_woken = higher_priority_task_woken;

        if (status == NTOS_OK)
        {
            g_queue_isr_send_count++;
        }
        else
        {
            g_queue_isr_full_count++;
        }
        /* ntos_queue_send_from_isr() đã tự request PendSV nếu cần - không làm gì thêm. */
    }
}

int main(void)
{
    ntos_status_t status_queue;
    ntos_status_t status_parser;
    ntos_status_t status_low;

    board_init();
    ntos_init();

    status_queue = ntos_queue_init_static(&event_queue, event_queue_storage, sizeof(uint32_t), EVENT_QUEUE_CAPACITY);

    status_parser = ntos_task_create_static(&task_parser_tcb, "parser", task_parser,
        NULL, task_parser_stack, STACK_WORDS, PARSER_PRIORITY);
    status_low = ntos_task_create_static(&task_low_tcb, "low", task_low,
        NULL, task_low_stack, STACK_WORDS, LOW_PRIORITY);

    if ((status_queue != NTOS_OK) || (status_parser != NTOS_OK) || (status_low != NTOS_OK))
    {
        for (;;)
        {
        }
    }

    ntos_start(&task_parser_tcb);

    for (;;)
    {
    }
}
