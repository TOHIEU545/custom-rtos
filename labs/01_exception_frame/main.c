#include <stdint.h>

#include "stm32f10x.h"

/*
 * Basic exception frame do Cortex-M3 tự động tạo khi exception xảy ra.
 */
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
volatile uint32_t g_main_counter;

__attribute__((noinline))
void systick_handler_c(exception_frame_t *frame,
                              uint32_t exc_return,
                              uint32_t msp,
                              uint32_t psp);

__attribute__((naked))
void SysTick_Handler(void)
{
    __asm volatile
    (
        "mrs     r0, msp             \n"
        "mov     r1, lr              \n"
        "mrs     r2, msp             \n"
        "mrs     r3, psp             \n"
        "b       systick_handler_c   \n"
    );
}

__attribute__((noinline))
void systick_handler_c(exception_frame_t *frame,
                              uint32_t exc_return,
                              uint32_t msp,
                              uint32_t psp)
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

int main(void)
{
    g_main_counter = 0u;

    systick_init();

    for (;;)
    {
        g_main_counter++;
        __NOP();
    }
}