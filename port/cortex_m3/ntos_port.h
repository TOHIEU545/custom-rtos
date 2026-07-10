#ifndef NTOS_PORT_H
#define NTOS_PORT_H
#include <stdint.h>
typedef uint32_t ntos_irq_state_t;
void ntos_port_request_context_switch(void);
void ntos_port_start_scheduler(void) __attribute__((noreturn));
ntos_irq_state_t ntos_port_irq_save(void);
void ntos_port_irq_restore(ntos_irq_state_t state);
#endif
