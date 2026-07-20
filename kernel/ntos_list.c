/**
 * @file ntos_list.c
 * @brief Intrusive doubly-linked circular list dùng sentinel - nền tảng cho ready/wait list.
 */
#include "ntos_assert_internal.h"
#include "ntos_list_internal.h"

#include <stddef.h>

void ntos_list_init(ntos_list_t *list)
{
    NTOS_ASSERT(list != NULL);

    /* Sentinel trỏ vòng về chính nó - list rỗng được biểu diễn bằng vòng lặp
     * 1-node này, không dùng NULL ở đâu cả. */
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
    item->container = NULL; /* chưa thuộc list nào */
}

void ntos_list_push_back(ntos_list_t *list, ntos_list_item_t *item)
{
    ntos_list_item_t *last;

    NTOS_ASSERT(list != NULL);
    NTOS_ASSERT(item != NULL);
    NTOS_ASSERT(item != &list->end);      /* không được chèn sentinel như item thường */
    NTOS_ASSERT(item->container == NULL); /* không được chèn trùng (double insert) */

    /* Chèn item vào ngay trước sentinel - tức cuối danh sách các item thực. */
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
    NTOS_ASSERT(item->container !=
                NULL); /* không được gỡ trùng / gỡ item chưa nằm trong list nào */

    list = item->container;
    NTOS_ASSERT(item != &list->end); /* không được gỡ sentinel */

    /* Gỡ item khỏi vòng: hai hàng xóm nối thẳng với nhau. */
    item->next->previous = item->previous;
    item->previous->next = item->next;

    /* Nếu index đang trỏ vào item sắp bị xóa, lùi index về item trước đó -
     * lần gọi ntos_list_next_owner() kế tiếp sẽ tiến từ đây, rơi đúng vào
     * item đã thay thế vị trí (do bước trên đã nối lại hàng xóm). */
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
        /* Vừa chạm sentinel - bỏ qua, vòng lại item đầu tiên (hoặc ở lại
         * sentinel nếu list đang rỗng). */
        list->index = list->index->next;
    }

    /* Không bao giờ được trả sentinel như item thật - nếu nó vào đây nghĩa là
     * list đang rỗng, lỗi của caller (phải tự kiểm tra ntos_list_is_empty()
     * trước khi gọi hàm này). */
    NTOS_ASSERT(list->index != &list->end);

    return list->index->owner;
}

bool ntos_list_is_empty(const ntos_list_t *list)
{
    NTOS_ASSERT(list != NULL);

    return list->length == 0U;
}
