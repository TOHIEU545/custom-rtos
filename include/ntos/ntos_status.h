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
    NTOS_OK = 0,               /**< Thành công. */
    NTOS_ERR_INVALID_ARGUMENT, /**< Tham số truyền vào không hợp lệ. */
    NTOS_ERR_INVALID_STATE,    /**< Đối tượng đang ở trạng thái không cho phép thao tác này. */
    NTOS_ERR_INVALID_PRIORITY, /**< Priority vượt quá giới hạn cho phép. */
    NTOS_ERR_INVALID_CONTEXT,  /**< Gọi sai ngữ cảnh, ví dụ API task gọi từ ISR. */
    NTOS_ERR_TIMEOUT,          /**< Hết thời gian chờ trước khi thao tác hoàn tất. */
    NTOS_ERR_QUEUE_FULL,       /**< Queue đã đầy, không thể gửi thêm item. */
    NTOS_ERR_QUEUE_EMPTY,      /**< Queue rỗng, không có item để nhận. */
    NTOS_ERR_RESOURCE_FULL,    /**< Tài nguyên (vd. semaphore) đã đạt giới hạn tối đa. */
    NTOS_ERR_INTERNAL          /**< Lỗi nội bộ kernel, không nên xảy ra trong vận hành bình thường. */
} ntos_status_t;

#endif