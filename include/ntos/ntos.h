#ifndef NTOS_H
#define NTOS_H
#include <ntos/ntos_queue.h>
#include <ntos/ntos_sem.h>
#include <ntos/ntos_task.h>
void ntos_init(void);
void ntos_start(void) __attribute__((noreturn));
ntos_tick_t ntos_tick_get(void);
#endif
