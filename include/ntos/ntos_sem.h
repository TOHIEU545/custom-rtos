#ifndef NTOS_SEM_H
#define NTOS_SEM_H
/**
 * @file ntos_sem.h
 * @brief API công khai semaphore đếm dùng để đồng bộ giữa task/ISR.
 */

#include <stdbool.h>
#include <stdint.h>
#include <ntos/ntos_status.h>
#include <ntos/ntos_types.h>

/**
 * @brief Semaphore đếm dùng chung giữa task và ISR.
 */
typedef struct ntos_semaphore ntos_semaphore_t;

/**
 * @brief Khởi tạo semaphore với số đếm ban đầu và tối đa.
 * @param sem Semaphore tĩnh do caller cấp.
 * @param initial_count Số đếm khởi tạo.
 * @param max_count Số đếm tối đa semaphore có thể đạt tới.
 */
void ntos_sem_init(ntos_semaphore_t *sem, uint32_t initial_count, uint32_t max_count);

/**
 * @brief Lấy semaphore, block nếu số đếm đang bằng 0.
 * @param sem Semaphore cần lấy.
 * @param timeout Số tick tối đa chờ nếu chưa lấy được.
 * @return NTOS_OK nếu lấy thành công, NTOS_ERR_TIMEOUT nếu hết thời gian chờ.
 */
ntos_status_t ntos_sem_take(ntos_semaphore_t *sem, ntos_tick_t timeout);

/**
 * @brief Trả semaphore, tăng số đếm và đánh thức task đang chờ nếu có.
 * @param sem Semaphore cần trả.
 * @return NTOS_OK nếu thành công, NTOS_ERR_RESOURCE_FULL nếu đã đạt max_count.
 */
ntos_status_t ntos_sem_give(ntos_semaphore_t *sem);

/**
 * @brief Trả semaphore từ ISR, không block.
 * @param sem Semaphore cần trả.
 * @param higher_priority_task_woken Set true nếu có task priority cao hơn được đánh thức.
 * @return NTOS_OK nếu thành công, NTOS_ERR_RESOURCE_FULL nếu đã đạt max_count.
 */
ntos_status_t ntos_sem_give_from_isr(ntos_semaphore_t *sem, bool *higher_priority_task_woken);
#endif
