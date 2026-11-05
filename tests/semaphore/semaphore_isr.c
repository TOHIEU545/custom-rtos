/*
 * Test semaphore wake-up from interrupt context.
 *
 * A high-priority task blocks on the semaphore while a lower-priority
 * CPU-bound task runs. The interrupt gives the semaphore and wakes the high
 * task, allowing the test to verify deferred preemption through PendSV.
 *
 * task_low chạy CPU-bound thay vì idle để chứng minh điều mạnh hơn: nút bấm
 * phải giành lại CPU từ một task đang thực sự RUNNING, không chỉ đánh thức
 * task_high lúc CPU đang rảnh (điều đó không chứng minh được cơ chế preempt
 * hoạt động).
 */
#include <rtos.h>
#include <bsp.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define STACK_WORDS ((size_t)64U)
#define HIGH_PRIORITY ((rtos_priority_t)5U)
#define LOW_PRIORITY ((rtos_priority_t)1U)

static rtos_tcb_t task_high_tcb;
static rtos_tcb_t task_low_tcb;
static _Alignas(8) uint32_t task_high_stack[STACK_WORDS];
static _Alignas(8) uint32_t task_low_stack[STACK_WORDS];

static rtos_semaphore_t sem;

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
        rtos_status_t status = rtos_sem_take(&sem, RTOS_WAIT_FOREVER);

        g_sem_isr_wake_count++;
        board_led_toggle(BOARD_LED_D0);

        bsp_uart_write_string("[9A ISR] semaphore wake #");
        uart_write_uint32(g_sem_isr_wake_count);
        bsp_uart_write_string(" take_status_ok=");
        bsp_uart_write_string((status == RTOS_OK) ? "true" : "false");
        bsp_uart_write_string(" higher_priority_task_woken=");
        bsp_uart_write_string(g_higher_priority_task_woken ? "true" : "false");
        bsp_uart_write_string("\r\n");
    }
}

static void task_low(void *argument)
{
    (void)argument;

    /* Bật NVIC EXTI1 ngay khi task này bắt đầu chạy, không phải trong
     * main()/trước rtos_start(): nếu nút được bấm sớm hơn - trước khi có bất
     * kỳ task nào chờ - rtos_sem_give_from_isr() sẽ chỉ tăng count thay vì
     * đánh thức ai, và lần bấm đó coi như bị bỏ lỡ khỏi kịch bản test thay vì
     * được quan sát như một sự kiện preempt thật. */
    bsp_exti_config(BOARD_KEY0_PORT, BOARD_KEY0_PIN, BSP_EXTI_TRIGGER_FALLING);

    /* Không delay: task này phải luôn ở trạng thái RUNNING để việc bấm nút
     * chứng minh được task_high thật sự preempt một task đang chạy, không
     * phải chỉ đánh thức nó lúc CPU rảnh. */
    for (;;)
    {
        g_low_run_count++;
    }
}

/* Dùng rtos_sem_give_from_isr() thay vì rtos_sem_give(): ISR không có TCB
 * riêng để scheduler đưa vào ready/wait list, và không được phép tự chọn
 * task kế tiếp hay tự đổi ngữ cảnh - nó chỉ có thể chuyển waiter sang READY
 * rồi yêu cầu PendSV xử lý phần switch thật sau khi ISR kết thúc. ISR chỉ
 * làm đúng hai việc (clear pending, give) rồi return ngay - không gọi thêm gì
 * khác để giữ thời gian ở mức ưu tiên ngắt tối thiểu. */
void EXTI1_IRQHandler(void)
{
    bool higher_priority_task_woken = false;

    bsp_exti_clear_pending(BOARD_KEY0_PIN);

    (void)rtos_sem_give_from_isr(&sem, &higher_priority_task_woken);
    g_higher_priority_task_woken = higher_priority_task_woken;
}

int main(void)
{
    rtos_status_t status_high;
    rtos_status_t status_low;

    board_init();
    rtos_init();

    rtos_sem_init(&sem, 0U, 1U);

    status_high = rtos_task_create_static(&task_high_tcb, "high", task_high,
        NULL, task_high_stack, STACK_WORDS, HIGH_PRIORITY);
    status_low = rtos_task_create_static(&task_low_tcb, "low", task_low,
        NULL, task_low_stack, STACK_WORDS, LOW_PRIORITY);

    if ((status_high != RTOS_OK) || (status_low != RTOS_OK))
    {
        for (;;)
        {
        }
    }

    rtos_start(&task_high_tcb, board_cpu_clock_hz());

    for (;;)
    {
    }
}
