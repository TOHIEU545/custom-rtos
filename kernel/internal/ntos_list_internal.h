#ifndef NTOS_LIST_INTERNAL_H
#define NTOS_LIST_INTERNAL_H
#include <stdbool.h>
#include <stdint.h>
struct ntos_list;
typedef struct ntos_list_item {
    struct ntos_list_item *next;
    struct ntos_list_item *previous;
    void *owner;
    struct ntos_list *container;
} ntos_list_item_t;
typedef struct ntos_list {
    ntos_list_item_t end;
    ntos_list_item_t *index;
    uint32_t length;
} ntos_list_t;
void ntos_list_init(ntos_list_t *list);
void ntos_list_item_init(ntos_list_item_t *item, void *owner);
void ntos_list_push_back(ntos_list_t *list, ntos_list_item_t *item);
void ntos_list_remove(ntos_list_item_t *item);
void *ntos_list_next_owner(ntos_list_t *list);
bool ntos_list_is_empty(const ntos_list_t *list);
#endif
