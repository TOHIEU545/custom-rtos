/**
 * @file test_initial_stack.c
 * @brief Test đơn giản cho ntos_task_create_static().
 */

#include <ntos/ntos_config_default.h>
#include <ntos/ntos_task.h>

#include <stddef.h>
#include <stdint.h>

#define TEST_STACK_WORDS 64U
#define TEST_FRAME_WORDS 16U
#define TEST_STACK_FILL  0xA5A5A5A5U

static ntos_tcb_t test_tcb;
static uint32_t test_stack[TEST_STACK_WORDS] __attribute__((aligned(8)));
static uint32_t test_argument = 0x12345678U;

volatile uint32_t test_finished;
volatile uint32_t test_failures;
volatile uint32_t test_failed_line;

#define TEST_ASSERT(condition)                  \
    do                                          \
    {                                           \
        if (!(condition))                       \
        {                                       \
            ++test_failures;                    \
            test_failed_line = (uint32_t)__LINE__; \
        }                                       \
    } while (0)

static void test_task(void *argument)
{
    (void)argument;

    for (;;)
    {
    }
}

int main(void)
{
    ntos_status_t status;
    uint32_t *frame;
    size_t i;

    test_finished = 0U;
    test_failures = 0U;
    test_failed_line = 0U;

    /*
     * test_tcb là biến static nên ban đầu toàn bộ field bằng 0.
     * NTOS_TASK_UNUSED cũng bằng 0.
     */
    status = ntos_task_create_static(
        &test_tcb,
        "test",
        test_task,
        &test_argument,
        test_stack,
        TEST_STACK_WORDS,
        1U);

    TEST_ASSERT(status == NTOS_OK);

    /* Kiểm tra metadata của TCB. */
    TEST_ASSERT(test_tcb.stack_low == test_stack);
    TEST_ASSERT(test_tcb.stack_high == test_stack + TEST_STACK_WORDS);
    TEST_ASSERT(test_tcb.stack_words == TEST_STACK_WORDS);

    TEST_ASSERT(
        test_tcb.stack_pointer ==
        test_stack + TEST_STACK_WORDS - TEST_FRAME_WORDS);

    TEST_ASSERT(test_tcb.entry == test_task);
    TEST_ASSERT(test_tcb.argument == &test_argument);
    TEST_ASSERT(test_tcb.priority == 1U);
    TEST_ASSERT(test_tcb.state == NTOS_TASK_READY);

    TEST_ASSERT(
        (((uintptr_t)test_tcb.stack_pointer & (uintptr_t)0x7U) == 0U));

    /*
     * Layout tính từ stack_pointer:
     *
     * [0]  R4
     * [1]  R5
     * [2]  R6
     * [3]  R7
     * [4]  R8
     * [5]  R9
     * [6]  R10
     * [7]  R11
     * [8]  R0
     * [9]  R1
     * [10] R2
     * [11] R3
     * [12] R12
     * [13] LR
     * [14] PC
     * [15] xPSR
     */
    frame = test_tcb.stack_pointer;

    TEST_ASSERT(frame[0] == 0x04040404U);
    TEST_ASSERT(frame[1] == 0x05050505U);
    TEST_ASSERT(frame[2] == 0x06060606U);
    TEST_ASSERT(frame[3] == 0x07070707U);
    TEST_ASSERT(frame[4] == 0x08080808U);
    TEST_ASSERT(frame[5] == 0x09090909U);
    TEST_ASSERT(frame[6] == 0x10101010U);
    TEST_ASSERT(frame[7] == 0x11111111U);

    TEST_ASSERT(frame[8] == (uint32_t)(uintptr_t)&test_argument);
    TEST_ASSERT(frame[9] == 0x01010101U);
    TEST_ASSERT(frame[10] == 0x02020202U);
    TEST_ASSERT(frame[11] == 0x03030303U);
    TEST_ASSERT(frame[12] == 0x12121212U);

    /* LR là địa chỉ task_return_error(), chỉ cần kiểm tra không bằng 0. */
    TEST_ASSERT(frame[13] != 0U);

    TEST_ASSERT(frame[14] == (uint32_t)(uintptr_t)test_task);
    TEST_ASSERT(frame[15] == 0x01000000U);

#if NTOS_ENABLE_STACK_FILL
    /* Phần stack chưa dùng phải giữ fill pattern. */
    for (i = 0U; i < TEST_STACK_WORDS - TEST_FRAME_WORDS; ++i)
    {
        TEST_ASSERT(test_stack[i] == TEST_STACK_FILL);
    }
#else
    (void)i;
#endif

    /*
     * Test tạo lại cùng TCB.
     * TCB hiện đã ở trạng thái READY nên phải bị từ chối.
     */
    status = ntos_task_create_static(
        &test_tcb,
        "test-again",
        test_task,
        NULL,
        test_stack,
        TEST_STACK_WORDS,
        1U);

    TEST_ASSERT(status == NTOS_ERR_INVALID_STATE);

    test_finished = 1U;

    /*
     * Kiểm tra bằng GDB:
     *
     * p test_finished
     * p test_failures
     * p test_failed_line
     *
     * PASS khi:
     * test_finished = 1
     * test_failures = 0
     */
    for (;;)
    {
    }
}