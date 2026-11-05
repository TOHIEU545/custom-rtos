#ifndef RTOS_CORE_H
#define RTOS_CORE_H

#include <stdint.h>

typedef uint32_t rtos_tick_t;
typedef uint8_t rtos_priority_t;
typedef void (*rtos_task_entry_t)(void *argument);

#define RTOS_NO_WAIT ((rtos_tick_t)0U)
#define RTOS_WAIT_FOREVER ((rtos_tick_t)UINT32_MAX)

typedef enum
{
    RTOS_OK = 0,
    RTOS_ERR_INVALID_ARGUMENT,
    RTOS_ERR_INVALID_STATE,
    RTOS_ERR_INVALID_PRIORITY,
    RTOS_ERR_INVALID_CONTEXT,
    RTOS_ERR_TIMEOUT,
    RTOS_ERR_QUEUE_FULL,
    RTOS_ERR_QUEUE_EMPTY,
    RTOS_ERR_RESOURCE_FULL,
    RTOS_ERR_INTERNAL
} rtos_status_t;

struct rtos_tcb;

void rtos_init(void);
void rtos_start(struct rtos_tcb *first_task_tcb, uint32_t cpu_clock_hz)
    __attribute__((noreturn));
rtos_tick_t rtos_tick_get(void);

#endif
