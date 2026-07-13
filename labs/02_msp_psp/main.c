#include <stdint.h>

#include "stm32f10x.h"

typedef struct
{
    uint32_t r0;
    uint32_t r1;
    uint32_t r2;
    uint32_t r3;
    uint32_t r12;
    uint32_t lr;
    uint32_t pc;
    uint32_t xpsr;
} exception_frame_t;

volatile exception_frame_t g_last_frame;

/*
 * Stack riêng dùng cho PSP.
 *
 * Cortex-M yêu cầu stack căn chỉnh phù hợp.
 */
#define PSP_STACK_WORDS 128U

__attribute__((aligned(8)))
static uint32_t g_psp_stack[PSP_STACK_WORDS];

volatile uint32_t g_control_before;
volatile uint32_t g_control_after;

volatile uint32_t g_msp_before;
volatile uint32_t g_psp_before;

volatile uint32_t g_msp_after;
volatile uint32_t g_psp_after;

volatile uint32_t g_main_counter;

__attribute__((noinline))
void systick_handler_c(exception_frame_t *frame,
                       uint32_t exc_return,
                       uint32_t handler_msp,
                       uint32_t handler_psp);

__attribute__((naked))
void SysTick_Handler(void)
{
    __asm volatile
    (
        "tst     lr, #4              \n"
        "ite     eq                  \n"
        "mrseq   r0, msp             \n"
        "mrsne   r0, psp             \n"

        "mov     r1, lr              \n"
        "mrs     r2, msp             \n"
        "mrs     r3, psp             \n"

        "b       systick_handler_c   \n"
    );
}

__attribute__((noinline))
void systick_handler_c(exception_frame_t *frame,
                       uint32_t exc_return,
                       uint32_t handler_msp,
                       uint32_t handler_psp)
{
    g_last_frame.r0   = frame->r0;
    g_last_frame.r1   = frame->r1;
    g_last_frame.r2   = frame->r2;
    g_last_frame.r3   = frame->r3;
    g_last_frame.r12  = frame->r12;
    g_last_frame.lr   = frame->lr;
    g_last_frame.pc   = frame->pc;
    g_last_frame.xpsr = frame->xpsr;

    /*
     * Đặt breakpoint tại đây.
     */
    __NOP();
}

static void systick_init(void)
{
    SystemCoreClockUpdate();

    const uint32_t ticks_per_ms = SystemCoreClock / 1000UL;

    if (SysTick_Config(ticks_per_ms) != 0UL)
    {
        for (;;)
        {
            __NOP();
        }
    }
}

/*
 * Chuyển Thread mode từ MSP sang PSP.
 */
static void switch_thread_to_psp(void)
{
    /*
     * Stack tăng trưởng về địa chỉ thấp, nên PSP phải trỏ vào đỉnh vùng stack.
     */
    uint32_t *psp_top = &g_psp_stack[PSP_STACK_WORDS];

    /*
     * Đảm bảo căn chỉnh 8 byte (làm tròn xuống bội số gần nhất của 8)
     */
    psp_top = (uint32_t *)((uint32_t)psp_top & ~0x7UL);

    g_control_before = __get_CONTROL();
    g_msp_before = __get_MSP();
    g_psp_before = __get_PSP();

    /*
     * Gán stack pointer riêng cho PSP.
     */
    __set_PSP((uint32_t)psp_top);

    /*
     * CONTROL.SPSEL là bit 1:
     *
     *   0 → Thread mode dùng MSP
     *   1 → Thread mode dùng PSP
     */
    uint32_t control = __get_CONTROL();
    control |= (1UL << 1);
    __set_CONTROL(control);

    /*
     * Đồng bộ pipeline sau khi thay đổi CONTROL.
     */
    __ISB();

    g_control_after = __get_CONTROL();
    g_msp_after = __get_MSP();
    g_psp_after = __get_PSP();
}

int main(void)
{
    g_main_counter = 0UL;

    g_control_before = 0UL;
    g_control_after = 0UL;

    g_msp_before = 0UL;
    g_psp_before = 0UL;
    g_msp_after = 0UL;
    g_psp_after = 0UL;

    switch_thread_to_psp();

    systick_init();

    for (;;)
    {
        g_main_counter++;
        __NOP();
    }
}