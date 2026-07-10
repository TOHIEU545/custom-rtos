#include "ntos_port.h"
#include "core_cm3.h"
void ntos_port_request_context_switch(void) { SCB->ICSR = SCB_ICSR_PENDSVSET_Msk; __DSB(); __ISB(); }
ntos_irq_state_t ntos_port_irq_save(void) { uint32_t s = __get_PRIMASK(); __disable_irq(); return s; }
void ntos_port_irq_restore(ntos_irq_state_t state) { __set_PRIMASK(state); }
void ntos_port_start_scheduler(void) { for (;;) {} /* TODO(stage 3). */ }
