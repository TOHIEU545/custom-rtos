/*
 * Test queue send from interrupt context.
 *
 * A parser task blocks waiting for data while a lower-priority CPU-bound task
 * runs. The interrupt sends an event into the queue and wakes the parser,
 * verifying ISR-safe queue usage and deferred preemption.
 *
 * task_low chạy CPU-bound thay vì idle để chứng minh điều mạnh hơn: nút bấm
 * phải giành lại CPU từ một task đang thực sự RUNNING, không chỉ đánh thức
 * task_parser lúc CPU đang rảnh.
 *
 * KEY1 dùng vector EXTI15_10 (dùng chung line 10-15), khác KEY2 dùng EXTI1
 * riêng ở test semaphore - nên handler ở đây phải tự kiểm đúng line của mình
 * trước khi xử lý, xem lý do tại chỗ khai báo handler bên dưới.
 */
#include <rtos.h>
#include <bsp.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define STACK_WORDS ((size_t)64U)
#define PARSER_PRIORITY ((rtos_priority_t)5U)
#define LOW_PRIORITY ((rtos_priority_t)1U)
#define EVENT_QUEUE_CAPACITY ((size_t)4U)

static rtos_tcb_t task_parser_tcb;
static rtos_tcb_t task_low_tcb;
static _Alignas(8) uint32_t task_parser_stack[STACK_WORDS];
static _Alignas(8) uint32_t task_low_stack[STACK_WORDS];

static rtos_queue_t event_queue;
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
        rtos_status_t status = rtos_queue_receive(&event_queue, &event, RTOS_WAIT_FOREVER);

        g_queue_isr_receive_count++;
        board_led_toggle(BOARD_LED_D1);

        bsp_uart_write_string("[9B ISR] event = ");
        uart_write_uint32(event);
        bsp_uart_write_string(" status_ok=");
        bsp_uart_write_string((status == RTOS_OK) ? "true" : "false");
        bsp_uart_write_string(" higher_priority_task_woken=");
        bsp_uart_write_string(g_queue_higher_priority_task_woken ? "true" : "false");
        bsp_uart_write_string("\r\n");
    }
}

static void task_low(void *argument)
{
    (void)argument;

    /* Bật NVIC EXTI15_10 ngay khi task này bắt đầu chạy, không phải trong
     * main()/trước rtos_start(): nếu nút được bấm sớm hơn - trước khi có bất
     * kỳ task nào chờ - rtos_queue_send_from_isr() sẽ chỉ ghi item vào buffer
     * mà không đánh thức ai, và lần bấm đó coi như bị bỏ lỡ khỏi kịch bản
     * test thay vì được quan sát như một sự kiện preempt thật. */
    bsp_exti_config(BOARD_KEY1_PORT, BOARD_KEY1_PIN, BSP_EXTI_TRIGGER_FALLING);

    /* Không delay: task này phải luôn ở trạng thái RUNNING để việc bấm nút
     * chứng minh được task_parser thật sự preempt một task đang chạy, không
     * phải chỉ đánh thức nó lúc CPU rảnh. */
    for (;;)
    {
        g_low_run_count++;
    }
}

/* EXTI15_10 là một vector dùng chung cho toàn bộ line 10-15 - CPU nhảy vào
 * đây bất kể line nào trong nhóm đó pending, nên phải tự kiểm đúng line 13
 * (KEY1) trước khi xử lý; nếu không, một nguồn ngắt khác cùng nhóm (nếu sau
 * này có) sẽ bị xử lý nhầm như thể nút KEY1 vừa được bấm. Dùng
 * rtos_queue_send_from_isr() thay vì rtos_queue_send(): ISR không có TCB
 * riêng để scheduler đưa vào wait list và không được tự chọn task kế tiếp -
 * nó chỉ ghi item, chuyển waiter sang READY nếu có, rồi để PendSV xử lý phần
 * switch thật sau khi return. */
void EXTI15_10_IRQHandler(void)
{
    if (bsp_exti_pending(BOARD_KEY1_PIN))
    {
        bool higher_priority_task_woken = false;
        uint32_t event;
        rtos_status_t status;

        bsp_exti_clear_pending(BOARD_KEY1_PIN);

        g_button_event_counter++;
        event = g_button_event_counter;

        status = rtos_queue_send_from_isr(&event_queue, &event, &higher_priority_task_woken);
        g_queue_higher_priority_task_woken = higher_priority_task_woken;

        /* Đếm riêng thành công/đầy để phân biệt qua UART lúc debug: nếu
         * bấm nút dồn dập nhanh hơn tốc độ parser tiêu thụ, event_queue có
         * thể đầy và một số lần bấm sẽ không được ghi nhận - đây là hành vi
         * đúng của một queue có capacity hữu hạn, không phải lỗi. */
        if (status == RTOS_OK)
        {
            g_queue_isr_send_count++;
        }
        else
        {
            g_queue_isr_full_count++;
        }
    }
}

int main(void)
{
    rtos_status_t status_queue;
    rtos_status_t status_parser;
    rtos_status_t status_low;

    board_init();
    rtos_init();

    status_queue = rtos_queue_init_static(&event_queue, event_queue_storage, sizeof(uint32_t), EVENT_QUEUE_CAPACITY);

    status_parser = rtos_task_create_static(&task_parser_tcb, "parser", task_parser,
        NULL, task_parser_stack, STACK_WORDS, PARSER_PRIORITY);
    status_low = rtos_task_create_static(&task_low_tcb, "low", task_low,
        NULL, task_low_stack, STACK_WORDS, LOW_PRIORITY);

    if ((status_queue != RTOS_OK) || (status_parser != RTOS_OK) || (status_low != RTOS_OK))
    {
        for (;;)
        {
        }
    }

    rtos_start(&task_parser_tcb, board_cpu_clock_hz());

    for (;;)
    {
    }
}
