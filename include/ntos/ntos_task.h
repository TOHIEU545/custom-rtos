#ifndef NTOS_TASK_H
#define NTOS_TASK_H
/**
 * @file ntos_task.h
 * @brief API công khai quản lý task: TCB, tạo task tĩnh, yield, delay.
 */

#include <ntos/ntos_list.h>
#include <ntos/ntos_status.h>
#include <ntos/ntos_types.h>
#include <stddef.h>
#include <stdint.h>

/**
 * @brief Trạng thái vòng đời của một task.
 */
typedef enum
{
    NTOS_TASK_UNUSED = 0,           /**< Slot TCB chưa được tạo/dùng. */
    NTOS_TASK_READY,                /**< Sẵn sàng chạy, chờ scheduler chọn. */
    NTOS_TASK_RUNNING,              /**< Đang là task hiện hành. */
    NTOS_TASK_BLOCKED               /**< Đang chờ sự kiện/tick, chưa chạy được. */
} ntos_task_state_t;

/**
 * @brief Lý do cụ thể một task đang NTOS_TASK_BLOCKED - phân biệt "chờ hết
 *        tick" (ntos_tick_handler() được phép wake) với "chờ IPC" (chỉ
 *        semaphore/queue give()/send() mới được wake, wake_tick không liên
 *        quan). Không có giá trị này thì ntos_tick_handler() không có cách
 *        nào biết một task BLOCKED có phải đang chờ delay hay không.
 */
typedef enum
{
    NTOS_BLOCK_NONE = 0,             /**< Không block - dùng cho READY/RUNNING/UNUSED. */
    NTOS_BLOCK_DELAY,                /**< Block trong ntos_delay(), wake_tick có ý nghĩa. */
    NTOS_BLOCK_SEMAPHORE,            /**< Block trong ntos_sem_take(), chờ ntos_sem_give()/give_from_isr(). */
    NTOS_BLOCK_QUEUE_SEND,           /**< Block trong ntos_queue_send() vì queue đầy. */
    NTOS_BLOCK_QUEUE_RECEIVE         /**< Block trong ntos_queue_receive() vì queue rỗng. */
} ntos_block_reason_t;

/**
 * @brief Task Control Block - lưu context và metadata của một task.
 * @note stack_pointer là field đầu tiên để lưu/khôi phục trực tiếp ngữ cảnh.
 */
typedef struct ntos_tcb
{
    uint32_t *stack_pointer;            /**< SP hiện tại của task. */
    uint32_t *stack_low;                /**< Địa chỉ thấp nhất của vùng stack. */
    uint32_t *stack_high;               /**< Địa chỉ ngay sau word cao nhất (exclusive). */
    size_t stack_words;                 /**< Kích thước stack tính theo word (4 byte). */
    ntos_task_entry_t entry;            /**< Hàm entry của task. */
    void *argument;                     /**< Tham số truyền cho entry qua R0. */
    const char *name;                   /**< Tên task, dùng để debug. */
    ntos_priority_t priority;           /**< Mức ưu tiên, số càng lớn càng ưu tiên. */
    ntos_task_state_t state;            /**< Trạng thái hiện tại của task. */
    ntos_block_reason_t block_reason;   /**< Chỉ có ý nghĩa khi state == NTOS_TASK_BLOCKED - lý do cụ thể. */
    ntos_tick_t wake_tick;              /**< Chỉ có ý nghĩa khi block_reason == NTOS_BLOCK_DELAY. */
    ntos_list_item_t ready_item;        /**< Node trong ready_lists[priority]; container == NULL khi BLOCKED/UNUSED. */
    ntos_list_item_t wait_item;         /**< Node trong wait list của semaphore/queue; container == NULL khi không chờ event nào. */
} ntos_tcb_t;

/**
 * @brief Tạo task với TCB và stack cấp phát tĩnh.
 * @param tcb TCB tĩnh do caller cấp, phải ở trạng thái NTOS_TASK_UNUSED.
 * @param name Tên task, dùng để debug.
 * @param entry Hàm entry của task, không được NULL và không được return.
 * @param argument Tham số truyền vào entry qua R0.
 * @param stack Mảng stack tĩnh do caller cấp, phải 8-byte-aligned.
 * @param stack_words Số word của stack, phải chẵn và đủ chứa initial frame.
 * @param priority Mức ưu tiên, phải nhỏ hơn NTOS_MAX_PRIORITIES.
 * @return NTOS_OK nếu thành công, mã lỗi tương ứng nếu tham số không hợp lệ.
 */
ntos_status_t ntos_task_create_static(ntos_tcb_t *tcb, const char *name, ntos_task_entry_t entry,
                                      void *argument, uint32_t *stack, size_t stack_words,
                                      ntos_priority_t priority);

/**
 * @brief Nhường CPU cho task khác (round-robin). Chỉ request PendSV, không tự switch.
 * @note Chỉ gọi từ task context (Thread mode). Không block, không có tác dụng ISR.
 */
void ntos_yield(void);

/**
 * @brief Trì hoãn task hiện tại trong số tick chỉ định, không busy-wait.
 * @param ticks Số tick cần chờ. 0 tương đương ntos_yield(). Tối đa INT32_MAX
 *              (giới hạn của phép so sánh tick wraparound-safe) - assert nếu vượt.
 * @note Chỉ gọi từ task context (Thread mode), không gọi từ idle task.
 */
void ntos_delay(ntos_tick_t ticks);
#endif
