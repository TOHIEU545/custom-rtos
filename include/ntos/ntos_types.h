#ifndef NTOS_TYPES_H
#define NTOS_TYPES_H
#include <stdint.h>
typedef uint32_t ntos_tick_t;
typedef uint8_t ntos_priority_t;
typedef void (*ntos_task_entry_t)(void *argument);
#define NTOS_NO_WAIT ((ntos_tick_t)0U)
#define NTOS_WAIT_FOREVER ((ntos_tick_t)UINT32_MAX)
#endif
