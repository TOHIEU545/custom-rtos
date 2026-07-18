#ifndef NTOS_LIST_INTERNAL_H
#define NTOS_LIST_INTERNAL_H

/**
 * @file ntos_list_internal.h
 * @brief Header nội bộ kernel cho list - re-export struct công khai, khai báo API thao tác.
 */

#include <ntos/ntos_list.h>

#include <stdbool.h>

void ntos_list_init(ntos_list_t *list);

void ntos_list_item_init(ntos_list_item_t *item, void *owner);

void ntos_list_push_back(ntos_list_t *list, ntos_list_item_t *item);

void ntos_list_remove(ntos_list_item_t *item);

void *ntos_list_next_owner(ntos_list_t *list);

bool ntos_list_is_empty(const ntos_list_t *list);

#endif
