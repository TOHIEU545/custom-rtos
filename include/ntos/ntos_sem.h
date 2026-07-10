#ifndef NTOS_SEM_H
#define NTOS_SEM_H
#include <stdbool.h>
#include <stdint.h>
#include <ntos/ntos_status.h>
#include <ntos/ntos_types.h>
typedef struct ntos_semaphore ntos_semaphore_t;
void ntos_sem_init(ntos_semaphore_t *sem, uint32_t initial_count, uint32_t max_count);
ntos_status_t ntos_sem_take(ntos_semaphore_t *sem, ntos_tick_t timeout);
ntos_status_t ntos_sem_give(ntos_semaphore_t *sem);
ntos_status_t ntos_sem_give_from_isr(ntos_semaphore_t *sem, bool *higher_priority_task_woken);
#endif
