/*
 * Producer sends increasing values through a queue; consumer receives and
 * prints them - a plain FIFO usage demo, not a queue-edge-case test.
 */
#include <ntos/ntos.h>
#include <board.h>
#include <uart.h>
#include <stddef.h>
#include <stdint.h>

#define STACK_WORDS ((size_t)64U)
#define PRODUCER_PRIORITY ((ntos_priority_t)1U)
#define CONSUMER_PRIORITY ((ntos_priority_t)2U)
#define SEND_PERIOD_TICKS 500U

static ntos_tcb_t producer_tcb;
static ntos_tcb_t consumer_tcb;
static _Alignas(8) uint32_t producer_stack[STACK_WORDS];
static _Alignas(8) uint32_t consumer_stack[STACK_WORDS];

static ntos_queue_t queue;
static uint32_t queue_storage[1];

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

/* Consumer có priority cao hơn producer nên nhận ngay mỗi item vừa gửi,
 * giữ log [send]/[recv] xen kẽ thay vì gửi dồn trước khi ai đó nhận. */
static void producer_task(void *argument)
{
    uint32_t value = 0U;
    (void)argument;

    for (;;)
    {
        value++;
        bsp_uart_write_string("[send] ");
        uart_write_uint32(value);
        bsp_uart_write_string("\r\n");

        (void)ntos_queue_send(&queue, &value, NTOS_WAIT_FOREVER);
        ntos_delay(SEND_PERIOD_TICKS);
    }
}

static void consumer_task(void *argument)
{
    uint32_t value;
    (void)argument;

    for (;;)
    {
        (void)ntos_queue_receive(&queue, &value, NTOS_WAIT_FOREVER);

        bsp_uart_write_string("[recv] ");
        uart_write_uint32(value);
        bsp_uart_write_string("\r\n");
    }
}

int main(void)
{
    ntos_status_t status_queue;
    ntos_status_t status_producer;
    ntos_status_t status_consumer;

    board_init();
    ntos_init();

    status_queue = ntos_queue_init_static(&queue, queue_storage, sizeof(uint32_t), 1U);

    if (status_queue != NTOS_OK)
    {
        for (;;)
        {
        }
    }

    status_producer = ntos_task_create_static(&producer_tcb, "producer", producer_task, NULL,
        producer_stack, STACK_WORDS, PRODUCER_PRIORITY);
    status_consumer = ntos_task_create_static(&consumer_tcb, "consumer", consumer_task, NULL,
        consumer_stack, STACK_WORDS, CONSUMER_PRIORITY);

    if ((status_producer != NTOS_OK) || (status_consumer != NTOS_OK))
    {
        for (;;)
        {
        }
    }

    ntos_start(&consumer_tcb);

    for (;;)
    {
    }
}
