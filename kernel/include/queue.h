#ifndef RTOS_QUEUE_H
#define RTOS_QUEUE_H

#include "core.h"
#include "list.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct rtos_queue
{
    uint8_t *storage;
    size_t item_size;
    size_t capacity;
    size_t read_index;
    size_t write_index;
    size_t count;
    rtos_list_t waiting_receivers;
    rtos_list_t waiting_senders;
} rtos_queue_t;

rtos_status_t rtos_queue_init_static(rtos_queue_t *queue, void *storage,
                                     size_t item_size, size_t capacity);
rtos_status_t rtos_queue_send(rtos_queue_t *queue, const void *item,
                              rtos_tick_t timeout);
rtos_status_t rtos_queue_receive(rtos_queue_t *queue, void *item,
                                 rtos_tick_t timeout);
rtos_status_t rtos_queue_send_from_isr(rtos_queue_t *queue, const void *item,
                                       bool *higher_priority_task_woken);

#endif
