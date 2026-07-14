#ifndef NTOS_STATUS_H
#define NTOS_STATUS_H
/**
 * @file ntos_status.h
 * @brief Mã trạng thái trả về dùng chung cho toàn bộ public API của NTOS.
 */

/**
 * @brief Các mã trạng thái trả về của NTOS.
 */
typedef enum
{
    NTOS_OK = 0,
    NTOS_ERR_INVALID_ARGUMENT,
    NTOS_ERR_INVALID_STATE,
    NTOS_ERR_INVALID_PRIORITY,
    NTOS_ERR_INVALID_CONTEXT,
    NTOS_ERR_TIMEOUT,
    NTOS_ERR_QUEUE_FULL,
    NTOS_ERR_QUEUE_EMPTY,
    NTOS_ERR_RESOURCE_FULL,
    NTOS_ERR_INTERNAL
} ntos_status_t;

#endif