#ifndef NTOS_QUEUE_H
#define NTOS_QUEUE_H
/**
 * @file ntos_queue.h
 * @brief API công khai hàng đợi message tĩnh giữa task/ISR.
 */

#include <stdbool.h>
#include <stddef.h>
#include <ntos/ntos_status.h>
#include <ntos/ntos_types.h>

/**
 * @brief Hàng đợi message tĩnh dùng chung giữa task và ISR.
 */
typedef struct ntos_queue ntos_queue_t;

/**
 * @brief Khởi tạo queue với vùng chứa item cấp phát tĩnh.
 * @param queue Queue tĩnh do caller cấp.
 * @param storage Vùng nhớ tĩnh chứa item, kích thước tối thiểu item_size * capacity.
 * @param item_size Kích thước (byte) của một item.
 * @param capacity Số item tối đa queue chứa được.
 * @return NTOS_OK nếu thành công, mã lỗi tương ứng nếu tham số không hợp lệ.
 */
ntos_status_t ntos_queue_init_static(ntos_queue_t *queue, void *storage, size_t item_size, size_t capacity);

/**
 * @brief Gửi một item vào queue, block nếu queue đầy.
 * @param queue Queue đích.
 * @param item Con trỏ tới dữ liệu cần gửi, được copy vào queue.
 * @param timeout Số tick tối đa chờ nếu queue đầy.
 * @return NTOS_OK nếu gửi thành công, NTOS_ERR_TIMEOUT/NTOS_ERR_QUEUE_FULL nếu không.
 */
ntos_status_t ntos_queue_send(ntos_queue_t *queue, const void *item, ntos_tick_t timeout);

/**
 * @brief Nhận một item từ queue, block nếu queue rỗng.
 * @param queue Queue nguồn.
 * @param item Nơi copy dữ liệu nhận được ra.
 * @param timeout Số tick tối đa chờ nếu queue rỗng.
 * @return NTOS_OK nếu nhận thành công, NTOS_ERR_TIMEOUT/NTOS_ERR_QUEUE_EMPTY nếu không.
 */
ntos_status_t ntos_queue_receive(ntos_queue_t *queue, void *item, ntos_tick_t timeout);

/**
 * @brief Gửi một item vào queue từ ISR, không block.
 * @param queue Queue đích.
 * @param item Con trỏ tới dữ liệu cần gửi, được copy vào queue.
 * @param higher_priority_task_woken Set true nếu có task priority cao hơn được đánh thức.
 * @return NTOS_OK nếu gửi thành công, NTOS_ERR_QUEUE_FULL nếu queue đầy.
 */
ntos_status_t ntos_queue_send_from_isr(ntos_queue_t *queue, const void *item, bool *higher_priority_task_woken);
#endif
