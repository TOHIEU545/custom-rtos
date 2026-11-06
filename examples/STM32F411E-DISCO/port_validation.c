#include <bsp.h>
#include <rtos.h>

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define HEARTBEAT_STACK_WORDS ((size_t)128U)
#define FLOAT_TASK_STACK_WORDS ((size_t)256U)

#define HEARTBEAT_PRIORITY ((rtos_priority_t)1U)
#define FLOAT_TASK_PRIORITY ((rtos_priority_t)2U)

#define HEARTBEAT_PERIOD_TICKS ((rtos_tick_t)500U)
#define FLOAT_A_PERIOD_TICKS ((rtos_tick_t)100U)
#define FLOAT_B_PERIOD_TICKS ((rtos_tick_t)137U)

#define FLOAT_TEST_ITERATIONS 64U
#define FP_HIGH_REGISTER_COUNT 16U

typedef enum validation_failure
{
    VALIDATION_OK = 0,
    VALIDATION_FLOAT_A_ARITHMETIC,
    VALIDATION_FLOAT_A_REGISTERS,
    VALIDATION_FLOAT_B_ARITHMETIC,
    VALIDATION_FLOAT_B_REGISTERS,
} validation_failure_t;

static rtos_tcb_t heartbeat_tcb;
static rtos_tcb_t float_a_tcb;
static rtos_tcb_t float_b_tcb;

static _Alignas(8) uint32_t heartbeat_stack[HEARTBEAT_STACK_WORDS];
static _Alignas(8) uint32_t float_a_stack[FLOAT_TASK_STACK_WORDS];
static _Alignas(8) uint32_t float_b_stack[FLOAT_TASK_STACK_WORDS];

static volatile bool float_a_passed;
static volatile bool float_b_passed;
static volatile validation_failure_t validation_failure;

/* Distinct finite single-precision bit patterns make cross-task corruption
 * visible without performing arithmetic on the test values themselves. */
static const uint32_t float_a_register_pattern[FP_HIGH_REGISTER_COUNT] = {
    0x3F800001U, 0x3F800002U, 0x3F800003U, 0x3F800004U,
    0x3F800005U, 0x3F800006U, 0x3F800007U, 0x3F800008U,
    0x3F800009U, 0x3F80000AU, 0x3F80000BU, 0x3F80000CU,
    0x3F80000DU, 0x3F80000EU, 0x3F80000FU, 0x3F800010U,
};

static const uint32_t float_b_register_pattern[FP_HIGH_REGISTER_COUNT] = {
    0x40000001U, 0x40000002U, 0x40000003U, 0x40000004U,
    0x40000005U, 0x40000006U, 0x40000007U, 0x40000008U,
    0x40000009U, 0x4000000AU, 0x4000000BU, 0x4000000CU,
    0x4000000DU, 0x4000000EU, 0x4000000FU, 0x40000010U,
};

static bool float_arithmetic_ok(float seed, float expected)
{
    volatile float value = seed;
    uint32_t iteration;

    /* All constants are exact binary fractions.  Each loop increases value by
     * exactly 0.25f while forcing real VFP loads and operations. */
    for (iteration = 0U; iteration < FLOAT_TEST_ITERATIONS; iteration++)
    {
        value = (value + 0.25f) * 2.0f;
        value *= 0.5f;
    }

    return value == expected;
}

static bool fp_high_registers_survive_switch(const uint32_t *pattern)
{
    uint32_t observed[FP_HIGH_REGISTER_COUNT];
    size_t index;

    /* S16-S31 are callee-saved by the hard-float ABI and are the part of the
     * FP context that PendSV must preserve in software. */
    __asm volatile("vldmia %0, {s16-s31}"
                   :
                   : "r"(pattern)
                   : "s16", "s17", "s18", "s19", "s20", "s21", "s22", "s23",
                     "s24", "s25", "s26", "s27", "s28", "s29", "s30", "s31",
                     "memory");

    /* Both float tasks have the same priority, so this yield selects the other
     * task and forces the high FP registers through PendSV save/restore. */
    rtos_yield();

    __asm volatile("vstmia %0, {s16-s31}"
                   :
                   : "r"(observed)
                   : "memory");

    for (index = 0U; index < FP_HIGH_REGISTER_COUNT; index++)
    {
        if (observed[index] != pattern[index])
        {
            return false;
        }
    }

    return true;
}

static void validation_fail(validation_failure_t failure)
{
    if (validation_failure == VALIDATION_OK)
    {
        validation_failure = failure;
    }

    board_led_set(BOARD_LED_RED, true);

    for (;;)
    {
        rtos_delay(1000U);
    }
}

static void print_failure(validation_failure_t failure)
{
    switch (failure)
    {
    case VALIDATION_FLOAT_A_ARITHMETIC:
        bsp_uart_write_string("validation: FAIL (float task A arithmetic)\r\n");
        break;
    case VALIDATION_FLOAT_A_REGISTERS:
        bsp_uart_write_string("validation: FAIL (float task A S16-S31)\r\n");
        break;
    case VALIDATION_FLOAT_B_ARITHMETIC:
        bsp_uart_write_string("validation: FAIL (float task B arithmetic)\r\n");
        break;
    case VALIDATION_FLOAT_B_REGISTERS:
        bsp_uart_write_string("validation: FAIL (float task B S16-S31)\r\n");
        break;
    case VALIDATION_OK:
    default:
        break;
    }
}

static void heartbeat_task(void *argument)
{
    bool pass_reported = false;
    bool failure_reported = false;

    (void)argument;

    for (;;)
    {
        if (validation_failure != VALIDATION_OK)
        {
            board_led_set(BOARD_LED_GREEN, false);
            board_led_set(BOARD_LED_RED, true);

            if (!failure_reported)
            {
                print_failure(validation_failure);
                failure_reported = true;
            }
        }
        else
        {
            board_led_toggle(BOARD_LED_GREEN);

            if (float_a_passed && float_b_passed && !pass_reported)
            {
                bsp_uart_write_string("basic context: PASS\r\n");
                bsp_uart_write_string("floating-point arithmetic: PASS\r\n");
                bsp_uart_write_string("S16-S31 context: PASS\r\n");
                pass_reported = true;
            }
        }

        rtos_delay(HEARTBEAT_PERIOD_TICKS);
    }
}

static void float_a_task(void *argument)
{
    (void)argument;

    for (;;)
    {
        if (!float_arithmetic_ok(1.0f, 17.0f))
        {
            validation_fail(VALIDATION_FLOAT_A_ARITHMETIC);
        }
        if (!fp_high_registers_survive_switch(float_a_register_pattern))
        {
            validation_fail(VALIDATION_FLOAT_A_REGISTERS);
        }

        float_a_passed = true;
        board_led_toggle(BOARD_LED_ORANGE);
        rtos_delay(FLOAT_A_PERIOD_TICKS);
    }
}

static void float_b_task(void *argument)
{
    (void)argument;

    for (;;)
    {
        if (!float_arithmetic_ok(2.0f, 18.0f))
        {
            validation_fail(VALIDATION_FLOAT_B_ARITHMETIC);
        }
        if (!fp_high_registers_survive_switch(float_b_register_pattern))
        {
            validation_fail(VALIDATION_FLOAT_B_REGISTERS);
        }

        float_b_passed = true;
        board_led_toggle(BOARD_LED_BLUE);
        rtos_delay(FLOAT_B_PERIOD_TICKS);
    }
}

static void startup_failure(void)
{
    board_led_set(BOARD_LED_RED, true);
    bsp_uart_write_string("validation: FAIL (task creation)\r\n");

    for (;;)
    {
    }
}

int main(void)
{
    rtos_status_t heartbeat_status;
    rtos_status_t float_a_status;
    rtos_status_t float_b_status;

    board_init();
    bsp_uart_write_string("MyRTOS Cortex-M4F port validation\r\n");
    rtos_init();

    heartbeat_status = rtos_task_create_static(
        &heartbeat_tcb, "heartbeat", heartbeat_task, NULL,
        heartbeat_stack, HEARTBEAT_STACK_WORDS, HEARTBEAT_PRIORITY);
    float_a_status = rtos_task_create_static(
        &float_a_tcb, "float-a", float_a_task, NULL,
        float_a_stack, FLOAT_TASK_STACK_WORDS, FLOAT_TASK_PRIORITY);
    float_b_status = rtos_task_create_static(
        &float_b_tcb, "float-b", float_b_task, NULL,
        float_b_stack, FLOAT_TASK_STACK_WORDS, FLOAT_TASK_PRIORITY);

    if ((heartbeat_status != RTOS_OK) ||
        (float_a_status != RTOS_OK) ||
        (float_b_status != RTOS_OK))
    {
        startup_failure();
    }

    bsp_uart_write_string("scheduler: starting\r\n");
    rtos_start(&float_a_tcb, board_cpu_clock_hz());
}
