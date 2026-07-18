#ifndef NTOS_LIST_INTERNAL_H
#define NTOS_LIST_INTERNAL_H

/**
 * @file ntos_list_internal.h
 * @brief Header nội bộ kernel cho list - re-export struct công khai, khai báo API thao tác.
 */

#include <ntos/ntos_list.h>

#include <stdbool.h>

/**
 * @brief Khởi tạo list rỗng.
 * @param list Con trỏ tới list cần khởi tạo.
 */
void ntos_list_init(ntos_list_t *list);

/**
 * @brief Khởi tạo item, chưa gắn vào list nào.
 * @param item Con trỏ tới item cần khởi tạo.
 * @param owner Struct sở hữu item này, lưu lại để truy xuất qua ntos_list_next_owner().
 */
void ntos_list_item_init(ntos_list_item_t *item, void *owner);

/**
 * @brief Thêm item vào cuối list.
 * @param list List đích.
 * @param item Item cần thêm, phải chưa thuộc list nào.
 */
void ntos_list_push_back(ntos_list_t *list, ntos_list_item_t *item);

/**
 * @brief Gỡ item khỏi list đang chứa nó.
 * @param item Item cần gỡ, phải đang thuộc một list.
 */
void ntos_list_remove(ntos_list_item_t *item);

/**
 * @brief Tiến cursor round-robin tới item kế tiếp và trả về owner của nó.
 * @param list List cần duyệt, phải không rỗng.
 * @return Owner đã truyền vào ntos_list_item_init() khi khởi tạo item đó.
 */
void *ntos_list_next_owner(ntos_list_t *list);

/**
 * @brief Kiểm tra list có rỗng hay không.
 * @param list Con trỏ tới list.
 * @return true nếu list rỗng, ngược lại false.
 */
bool ntos_list_is_empty(const ntos_list_t *list);

#endif
