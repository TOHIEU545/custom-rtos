#ifndef NTOS_QUEUE_H
#define NTOS_QUEUE_H
#include <stdbool.h>
#include <stddef.h>
#include <ntos/ntos_status.h>
#include <ntos/ntos_types.h>
typedef struct ntos_queue ntos_queue_t;
ntos_status_t ntos_queue_init_static(ntos_queue_t *queue, void *storage, size_t item_size, size_t capacity);
ntos_status_t ntos_queue_send(ntos_queue_t *queue, const void *item, ntos_tick_t timeout);
ntos_status_t ntos_queue_receive(ntos_queue_t *queue, void *item, ntos_tick_t timeout);
ntos_status_t ntos_queue_send_from_isr(ntos_queue_t *queue, const void *item, bool *higher_priority_task_woken);
#endif
