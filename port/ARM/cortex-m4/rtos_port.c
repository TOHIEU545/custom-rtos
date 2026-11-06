#include "rtos_port.h"

#include "config.h"
#include "kernel_internal.h"

#include <stddef.h>
#include <stdint.h>

/* CMSIS-Core normally receives these definitions from a device header.  The
 * port deliberately does not include an STM32 header, so it remains usable by
 * any Cortex-M4F board.  No vendor IRQ is needed here. */
typedef enum IRQn
{
    NonMaskableInt_IRQn = -14,
    HardFault_IRQn = -13,
    MemoryManagement_IRQn = -12,
    BusFault_IRQn = -11,
    UsageFault_IRQn = -10,
    SVCall_IRQn = -5,
    DebugMonitor_IRQn = -4,
    PendSV_IRQn = -2,
    SysTick_IRQn = -1
} IRQn_Type;

#define __CM4_REV 0x0001U
#define __MPU_PRESENT 0U
#define __NVIC_PRIO_BITS 8U
#define __Vendor_SysTickConfig 0U
#define __FPU_PRESENT 1U

#include "core_cm4.h"

#if (__FPU_USED != 1U)
#error "The Cortex-M4F RTOS port must be compiled with hardware floating-point enabled"
#endif

_Static_assert(offsetof(struct rtos_tcb, stack_pointer) == 0U,
               "stack_pointer must be the first field in struct rtos_tcb");
_Static_assert(RTOS_PORT_INITIAL_FRAME_WORDS == 18U,
               "Cortex-M4F initial frame layout changed unexpectedly");

#define RTOS_TASK_INITIAL_XPSR 0x01000000U
#define RTOS_TASK_INITIAL_EXC_RETURN EXC_RETURN_THREAD_PSP

static void rtos_port_task_return_error(void)
{
    rtos_port_panic();
}

uint32_t *rtos_port_stack_init(uint32_t *stack, size_t stack_words,
                               rtos_task_entry_t entry, void *argument)
{
    uint32_t *sp = stack + stack_words;

    /* Basic exception frame, restored automatically by exception return. */
    *(--sp) = RTOS_TASK_INITIAL_XPSR;                      /* xPSR */
    *(--sp) = (uint32_t)(uintptr_t)entry;                  /* PC */
    *(--sp) = (uint32_t)(uintptr_t)rtos_port_task_return_error; /* LR */
    *(--sp) = 0x0000000CU;                                /* R12 */
    *(--sp) = 0x00000003U;                                /* R3 */
    *(--sp) = 0x00000002U;                                /* R2 */
    *(--sp) = 0x00000001U;                                /* R1 */
    *(--sp) = (uint32_t)(uintptr_t)argument;               /* R0 */

    /* Software frame.  EXC_RETURN bit 4 starts at one because a new task has
     * not used the FPU yet and therefore has no extended FP frame. */
    *(--sp) = RTOS_TASK_INITIAL_EXC_RETURN;                /* EXC_RETURN */
    *(--sp) = 0x0000000BU;                                /* R11 */
    *(--sp) = 0x0000000AU;                                /* R10 */
    *(--sp) = 0x00000009U;                                /* R9 */
    *(--sp) = 0x00000008U;                                /* R8 */
    *(--sp) = 0x00000007U;                                /* R7 */
    *(--sp) = 0x00000006U;                                /* R6 */
    *(--sp) = 0x00000005U;                                /* R5 */
    *(--sp) = 0x00000004U;                                /* R4 */
    *(--sp) = 0x00000003U;                                /* alignment padding */

    return sp;
}

void rtos_port_request_context_switch(void)
{
    SCB->ICSR = SCB_ICSR_PENDSVSET_Msk;
    __DSB();
    __ISB();
}

rtos_irq_state_t rtos_port_irq_save(void)
{
    uint32_t state = __get_PRIMASK();

    __disable_irq();
    return state;
}

void rtos_port_irq_restore(rtos_irq_state_t state)
{
    __set_PRIMASK(state);
}

static void rtos_port_enable_fpu(void)
{
    /* Grant privileged and unprivileged access to CP10 and CP11. */
    SCB->CPACR |= (3UL << 20U) | (3UL << 22U);
    __DSB();
    __ISB();

    /* Track FP use per Thread context and defer low-register stacking until
     * an exception handler actually executes an FP instruction. */
    FPU->FPCCR |= FPU_FPCCR_ASPEN_Msk | FPU_FPCCR_LSPEN_Msk;
}

void rtos_port_start_scheduler(void)
{
    rtos_port_enable_fpu();
    __enable_irq();

    __asm volatile("svc 0" ::: "memory");

    __builtin_unreachable();
}

void rtos_port_start_tick(uint32_t cpu_clock_hz)
{
    uint32_t reload;

    RTOS_ASSERT(cpu_clock_hz > 0U);
    reload = cpu_clock_hz / RTOS_TICK_RATE_HZ;
    RTOS_ASSERT(reload > 0U);
    RTOS_ASSERT((reload - 1U) <= SysTick_LOAD_RELOAD_Msk);

    /* PendSV cannot interrupt another handler.  SysTick is kept at the same
     * lowest priority; its pending PendSV runs only after SysTick returns. */
    SCB->SHP[10] = UINT8_MAX;
    SCB->SHP[11] = UINT8_MAX;

    SysTick->LOAD = reload - 1U;
    SysTick->VAL = 0U;
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_TICKINT_Msk |
                    SysTick_CTRL_ENABLE_Msk;
}

void rtos_port_panic(void)
{
    __disable_irq();
    for (;;)
    {
        __BKPT(0);
    }
}

__attribute__((naked)) void SVC_Handler(void)
{
    __asm volatile(
        "ldr   r0, =rtos_current_tcb\n"
        "ldr   r0, [r0]\n"
        "ldr   r0, [r0]\n"
        "ldmia r0!, {r3-r11, lr}\n"
        "msr   psp, r0\n"
        "ldr   r1, =rtos_scheduler_running\n"
        "movs  r2, #1\n"
        "strb  r2, [r1]\n"
        "bx    lr\n");
}

__attribute__((naked)) void PendSV_Handler(void)
{
    __asm volatile(
        "mrs       r0, psp\n"

        /* EXC_RETURN bit 4 is clear when the interrupted task owns an
         * extended hardware FP frame.  S0-S15 and FPSCR belong to that frame;
         * the port only has to preserve the callee-saved S16-S31 registers. */
        "tst       lr, #0x10\n"
        "it        eq\n"
        "vstmdbeq  r0!, {s16-s31}\n"
        "stmdb     r0!, {r3-r11, lr}\n"

        "ldr       r1, =rtos_current_tcb\n"
        "ldr       r2, [r1]\n"
        "str       r0, [r2]\n"

        /* Preserve exception LR on the handler MSP and keep the stack aligned
         * to eight bytes while the C scheduler runs. */
        "push      {r3, lr}\n"
        "bl        rtos_scheduler_select_next\n"
        "pop       {r3, lr}\n"

        "ldr       r1, =rtos_current_tcb\n"
        "ldr       r2, [r1]\n"
        "ldr       r0, [r2]\n"
        "ldmia     r0!, {r3-r11, lr}\n"
        "tst       lr, #0x10\n"
        "it        eq\n"
        "vldmiaeq  r0!, {s16-s31}\n"
        "msr       psp, r0\n"
        "bx        lr\n");
}

void SysTick_Handler(void)
{
    rtos_tick_handler();
}
