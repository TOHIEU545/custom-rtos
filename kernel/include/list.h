#ifndef RTOS_LIST_H
#define RTOS_LIST_H

#include <stdint.h>
#include <stdbool.h>

struct rtos_list;

typedef struct rtos_list_item
{
    struct rtos_list_item *next;
    struct rtos_list_item *previous;
    void *owner;
    struct rtos_list *container;
} rtos_list_item_t;

typedef struct rtos_list
{
    rtos_list_item_t end;
    rtos_list_item_t *index;
    uint32_t length;
} rtos_list_t;

void rtos_list_init(rtos_list_t *list);
void rtos_list_item_init(rtos_list_item_t *item, void *owner);
void rtos_list_push_back(rtos_list_t *list, rtos_list_item_t *item);
void rtos_list_remove(rtos_list_item_t *item);
void *rtos_list_next_owner(rtos_list_t *list);
bool rtos_list_is_empty(const rtos_list_t *list);

#endif
