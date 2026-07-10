#ifndef NTOS_KERNEL_INTERNAL_H
#define NTOS_KERNEL_INTERNAL_H
#include "ntos_task_internal.h"
extern struct ntos_tcb *volatile ntos_current_tcb;
void ntos_scheduler_select_next(void);
void ntos_tick_handler(void);
#endif
