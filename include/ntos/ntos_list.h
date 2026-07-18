#ifndef NTOS_LIST_H
#define NTOS_LIST_H
/**
 * @file ntos_list.h
 * @brief Layout công khai của intrusive doubly-linked circular list.
 * @note Chỉ export struct để TCB/semaphore/queue embed được item trực tiếp -
 *       các hàm thao tác list (push_back/remove/...) vẫn là kernel-internal,
 *       xem kernel/internal/ntos_list_internal.h.
 */
#include <stdint.h>

struct ntos_list;

/**
 * @brief Một node trong intrusive list - nhúng trực tiếp vào struct sở hữu (vd. TCB).
 */
typedef struct ntos_list_item
{
    struct ntos_list_item *next;     /**< Item kế tiếp, vòng lại sentinel nếu là cuối. */
    struct ntos_list_item *previous; /**< Item trước, vòng lại sentinel nếu là đầu. */
    void *owner;                     /**< Struct sở hữu item này (vd. ntos_tcb_t*). */
    struct ntos_list *container;     /**< List đang chứa item; NULL nếu chưa/không còn ở list nào. */
} ntos_list_item_t;

/**
 * @brief List tuần hoàn dùng sentinel `end`, có cursor `index` để round-robin.
 */
typedef struct ntos_list
{
    ntos_list_item_t end;           /**< Sentinel - không đại diện item thật, luôn tồn tại. */
    ntos_list_item_t *index;        /**< Vị trí round-robin đã duyệt tới lần gần nhất. */
    uint32_t length;                /**< Số item thật đang có (không tính sentinel). */
} ntos_list_t;

#endif
