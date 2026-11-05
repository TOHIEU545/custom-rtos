#include "rtos_port.h"

#include "kernel_internal.h"
#include "config.h"

#include <stddef.h>

/* CMSIS-Core requires the architectural exception type and implementation
 * constants normally supplied by a device header. This port does not use any
 * vendor interrupt, so only the Cortex-M exceptions are declared here. */
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

#define __CM3_REV 0x0200U
#define __MPU_PRESENT 0U
#define __NVIC_PRIO_BITS 8U
#define __Vendor_SysTickConfig 0U

#include "core_cm3.h"

_Static_assert(offsetof(struct rtos_tcb, stack_pointer) == 0U,
               "stack_pointer phai la field dau tien trong struct rtos_tcb");

#define RTOS_TASK_INITIAL_XPSR 0x01000000U

static void rtos_port_task_return_error(void)
{
    for (;;)
    {
    }
}

uint32_t *rtos_port_stack_init(uint32_t *stack, size_t stack_words,
                               rtos_task_entry_t entry, void *argument)
{
    uint32_t *sp = stack + stack_words;

    *(--sp) = RTOS_TASK_INITIAL_XPSR;
    *(--sp) = (uint32_t)entry;                       /* PC */
    *(--sp) = (uint32_t)rtos_port_task_return_error; /* LR */
    *(--sp) = 0x0000000CU;                           /* R12 */
    *(--sp) = 0x00000003U;                           /* R3 */
    *(--sp) = 0x00000002U;                           /* R2 */
    *(--sp) = 0x00000001U;                           /* R1 */
    *(--sp) = (uint32_t)argument;                    /* R0 */
    *(--sp) = 0x0000000BU;                           /* R11 */
    *(--sp) = 0x0000000AU;                           /* R10 */
    *(--sp) = 0x00000009U;                           /* R9 */
    *(--sp) = 0x00000008U;                           /* R8 */
    *(--sp) = 0x00000007U;                           /* R7 */
    *(--sp) = 0x00000006U;                           /* R6 */
    *(--sp) = 0x00000005U;                           /* R5 */
    *(--sp) = 0x00000004U;                           /* R4 */

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
    uint32_t s = __get_PRIMASK();
    __disable_irq();
    return s;
}
void rtos_port_irq_restore(rtos_irq_state_t state)
{
    __set_PRIMASK(state);
}

void rtos_port_start_scheduler(void)
{
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

    /* PendSV phải là exception ưu tiên thấp nhất - không được chen ngang bất kỳ
     * handler nào khác, context switch chỉ xảy ra khi không còn việc gì ưu tiên hơn. */
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
    __asm volatile("ldr   r0, =rtos_current_tcb\n"          /* r0 = &rtos_current_tcb */
                   "ldr   r0, [r0]\n"                       /* r0 = rtos_current_tcb = địa chỉ TCB */
                   "ldr   r0, [r0]\n"                       /* r0 = rtos_current_tcb->stack_pointer */
                   "ldmia r0!, {r4-r11}\n"                  /* R4-R11 được restore  */
                   "msr   psp, r0\n"                        /* PSP = dau hardware frame (slot R0) */
                   "ldr   r1, =rtos_scheduler_running\n"    /* Chi bao scheduler "running" SAU KHI PSP da hop le - */
                   "movs  r2, #1\n"                         /* neu SysTick pend PendSV som hon diem nay (vd. xen giua */
                   "strb  r2, [r1]\n"                       /* __enable_irq() va svc), PendSV_Handler van thay PSP dung. */
                   "ldr   lr, =0xFFFFFFFD\n"                /* EXC_RETURN: ve Thread mode, dung PSP */
                   "bx    lr\n"                             /* exception return - CPU tu pop R0-R3,R12,LR,PC,xPSR */
    );
}

__attribute__((naked)) void PendSV_Handler(void)
{
    __asm volatile(
        "mrs    r0, psp\n"                                  /* Lấy PSP của task đang chạy, lúc này PSP đang trỏ vào R0 trong exception frame */
        "stmdb  r0!, {r4-r11}\n"                            /* Lưu R4-R11 đồng mỗi lần lưu giảm PSP 32 bit (4 byte) */

        "ldr    r1, =rtos_current_tcb\n"                    /* r1 = &rtos_current_tcb */
        "ldr    r2, [r1]\n"                                 /* r2 = rtos_current_tcb = địa chỉ TCB */
        "str    r0, [r2]\n"                                 /* rtos_current_tcb->stack_pointer = r0 */

        "push   {r3, lr}\n"                                 /* Lưu EXC_RETURN trong lr và giữ MSP align 8 byte trước khi goin hàm C */
        "bl     rtos_scheduler_select_next\n"               /* Gọi scheduler C để chon task tiếp theo; BL tạm ghi đè lr */
        "pop    {r3, lr}\n"                                 /* Khôi phục EXC_RETURN ban đầu vao lr sau khi scheduler return */

        "ldr    r1, =rtos_current_tcb\n"                    /* r1 = &rtos_current_tcb (rtos_current_tcb có thể đã đổi sau hàm scheduler) */
        "ldr    r2, [r1]\n"                                 /* r2 = rtos_current_tcb = địa chỉ TCB */
        "ldr    r0, [r2]\n"                                 /* r0 = rtos_current_tcb->stack_pointer */

        "ldmia  r0!, {r4-r11}\n"                            /* R4-R11 được restore  */
        "msr    psp, r0\n"                                  /* PSP = dau hardware frame (slot R0) */
        "bx     lr\n"                                       /* exception return - CPU tu pop R0-R3,R12,LR,PC,xPSR */
    );
}

void SysTick_Handler(void)
{
    /* Handler chỉ gọi vào kernel - không tự tăng PSP/rtos_current_tcb, không tự
     * pend PendSV (xem rtos_tick_handler() trong kernel/src/core.c). */
    rtos_tick_handler();
}
