#include "ntos_port.h"

#include "ntos_kernel_internal.h"

#include "stm32f10x.h"

#include <stddef.h>

_Static_assert(offsetof(struct ntos_tcb, stack_pointer) == 0U,
               "stack_pointer phai la field dau tien trong struct ntos_tcb");

void ntos_port_request_context_switch(void)
{
    SCB->ICSR = SCB_ICSR_PENDSVSET_Msk;
    __DSB();
    __ISB();
}
ntos_irq_state_t ntos_port_irq_save(void)
{
    uint32_t s = __get_PRIMASK();
    __disable_irq();
    return s;
}
void ntos_port_irq_restore(ntos_irq_state_t state)
{
    __set_PRIMASK(state);
}

void ntos_port_start_scheduler(void)
{
    __enable_irq();

    __asm volatile("svc 0" ::: "memory");

    __builtin_unreachable();
}

__attribute__((naked)) void SVC_Handler(void)
{
    __asm volatile("ldr   r0, =ntos_current_tcb\n"  /* r0 = &ntos_current_tcb */
                   "ldr   r0, [r0]\n"               /* r0 = ntos_current_tcb = địa chỉ TCB */
                   "ldr   r0, [r0]\n"               /* r0 = ntos_current_tcb->stack_pointer */
                   "ldmia r0!, {r4-r11}\n"          /* R4-R11 được restore  */
                   "msr   psp, r0\n"                /* PSP = dau hardware frame (slot R0)             */
                   "ldr   lr, =0xFFFFFFFD\n"        /* EXC_RETURN: ve Thread mode, dung PSP            */
                   "bx    lr\n"                     /* exception return - CPU tu pop R0-R3,R12,LR,PC,xPSR */
    );
}
