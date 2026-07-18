/**
 * @file ntos_list.c
 * @brief Intrusive doubly-linked circular list dùng sentinel - nền tảng cho ready/wait list (Chặng 8+).
 */
#include "ntos_assert_internal.h"
#include "ntos_list_internal.h"

#include <stddef.h>

void ntos_list_init(ntos_list_t *list)
{
    NTOS_ASSERT(list != NULL);

    /* Sentinel tro vong ve chinh no - list rong duoc bieu dien bang vong lap
     * 1-node nay, khong dung NULL o dau ca. */
    list->end.next = &list->end;
    list->end.previous = &list->end;
    list->end.owner = NULL;
    list->end.container = list;

    list->index = &list->end;
    list->length = 0U;
}

void ntos_list_item_init(ntos_list_item_t *item, void *owner)
{
    NTOS_ASSERT(item != NULL);

    item->next = NULL;
    item->previous = NULL;
    item->owner = owner;
    item->container = NULL; /* chua thuoc list nao */
}

void ntos_list_push_back(ntos_list_t *list, ntos_list_item_t *item)
{
    ntos_list_item_t *last;

    NTOS_ASSERT(list != NULL);
    NTOS_ASSERT(item != NULL);
    NTOS_ASSERT(item != &list->end);   /* khong duoc chen sentinel nhu item thuong */
    NTOS_ASSERT(item->container == NULL); /* khong duoc double insert */

    /* Chen item vao ngay truoc sentinel - tuc la cuoi danh sach cac item thuc. */
    last = list->end.previous;

    item->next = &list->end;
    item->previous = last;

    last->next = item;
    
    list->end.previous = item;

    item->container = list;
    list->length++;
}

void ntos_list_remove(ntos_list_item_t *item)
{
    ntos_list_t *list;

    NTOS_ASSERT(item != NULL);
    NTOS_ASSERT(item->container != NULL); /* khong duoc double remove / remove item chua o trong list */

    list = item->container;
    NTOS_ASSERT(item != &list->end); /* khong duoc remove sentinel */

    /* Boc item ra khoi vong: hai hang xom tu noi thang voi nhau. */
    item->next->previous = item->previous;
    item->previous->next = item->next;

    /* Neu index dang tro vao item sap bi xoa, lui index ve item truoc do -
     * lan goi ntos_list_next_owner() ke tiep se tien tu day, roi vao dung
     * item da thay the vi tri (do buoc tren da noi lai hang xom). */
    if (list->index == item)
    {
        list->index = item->previous;
    }

    item->next = NULL;
    item->previous = NULL;
    item->container = NULL;

    list->length--;
}

void *ntos_list_next_owner(ntos_list_t *list)
{
    NTOS_ASSERT(list != NULL);

    list->index = list->index->next;
    if (list->index == &list->end)
    {
        /* Vua cham sentinel - bo qua, vong lai item dau tien (hoac o lai
         * sentinel neu list dang rong). */
        list->index = list->index->next;
    }

    /* Khong bao gio duoc tra sentinel nhu item that - neu no vao day nghia la
     * list dang rong, loi cua caller (phai tu kiem tra ntos_list_is_empty()
     * truoc khi goi ham nay). */
    NTOS_ASSERT(list->index != &list->end);

    return list->index->owner;
}

bool ntos_list_is_empty(const ntos_list_t *list)
{
    NTOS_ASSERT(list != NULL);

    return list->length == 0U;
}
