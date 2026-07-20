/**
 * @file ntos_kernel.c
 * @brief Khởi tạo kernel, scheduler theo priority, tick handler.
 */
#include <ntos/ntos.h>

#include <ntos/ntos_config_default.h>

#include "ntos_assert_internal.h"
#include "ntos_kernel_internal.h"
#include "ntos_list_internal.h"

#include "ntos_port.h"
#include <stddef.h>
#include <stdint.h>

struct ntos_tcb *volatile ntos_current_tcb = NULL;
volatile bool ntos_scheduler_running = false;
static volatile ntos_tick_t ntos_tick_count = 0U;

static ntos_tcb_t *task_table[NTOS_MAX_TASKS];
static size_t task_count = 0U;

/* Ready list theo từng priority - đại diện toàn bộ task RUNNABLE (RUNNING +
 * READY); BLOCKED/UNUSED không nằm trong đây. index của từng list tự làm
 * round-robin cursor riêng cho priority đó. Không "static" vì export qua
 * kernel_internal.h để test kiểm tra invariant container. */
ntos_list_t ready_lists[NTOS_MAX_PRIORITIES];

ntos_tcb_t ntos_idle_task_tcb;
static _Alignas(8) uint32_t ntos_idle_task_stack[NTOS_IDLE_STACK_WORDS];

/* Idle task: không làm gì cả, không block, không gọi semaphore/queue.
 * Priority 0 (thấp nhất) - chỉ được chọn khi không còn task application nào
 * priority cao hơn đang READY (xem quy ước round-robin cùng priority đã
 * quyết định trong ntos_scheduler_select_next()). */
static void ntos_idle_task(void *argument)
{
    (void)argument;

    for (;;)
    {
    }
}

void ntos_init(void)
{
    size_t p;
    ntos_status_t status;

    /* Phải init xong hết ready_lists[] TRƯỚC khi tạo task đầu tiên (kể cả idle) -
     * ntos_scheduler_register_task() sẽ ntos_list_push_back() vào đây ngay. */
    for (p = 0U; p < NTOS_MAX_PRIORITIES; ++p)
    {
        ntos_list_init(&ready_lists[p]);
    }

    status = ntos_task_create_static(&ntos_idle_task_tcb, "idle", ntos_idle_task, NULL,
                                     ntos_idle_task_stack, NTOS_IDLE_STACK_WORDS, 0U);

    NTOS_ASSERT(status == NTOS_OK);
}

void ntos_start(ntos_tcb_t *first_task_tcb)
{
    NTOS_ASSERT(ntos_current_tcb == NULL);
    NTOS_ASSERT(first_task_tcb != NULL);
    NTOS_ASSERT(first_task_tcb->state == NTOS_TASK_READY);
    NTOS_ASSERT(first_task_tcb->stack_pointer != NULL);
    NTOS_ASSERT(first_task_tcb->stack_pointer >= first_task_tcb->stack_low);
    NTOS_ASSERT(first_task_tcb->stack_pointer < first_task_tcb->stack_high);
    NTOS_ASSERT(((uintptr_t)first_task_tcb->stack_pointer & 0x7U) == 0U);
    /* Kiểm tra first_task đang nằm đúng ready_lists[priority] của nó - invariant
     * RUNNABLE task phải có ready_item.container trỏ đúng list. */
    NTOS_ASSERT(first_task_tcb->ready_item.container == &ready_lists[first_task_tcb->priority]);
    /* RUNNING task không được mang theo dấu vết của một lần BLOCKED nào - xem
     * cùng invariant được assert lại ở ntos_scheduler_select_next(). */
    NTOS_ASSERT(first_task_tcb->block_reason == NTOS_BLOCK_NONE);
    NTOS_ASSERT(first_task_tcb->wait_item.container == NULL);

    first_task_tcb->state = NTOS_TASK_RUNNING;
    ntos_current_tcb = first_task_tcb;

    /* Priority PendSV/SysTick và SysTick timer phải sẵn sàng trước khi bật interrupt.
     * ntos_scheduler_running KHÔNG được đặt true ở đây - SVC_Handler (port) tự đặt
     * true ngay sau khi PSP của first task hợp lệ, để loại bỏ hoàn toàn khả năng
     * SysTick pend PendSV trước khi PSP sẵn sàng (trỏ vào vùng nhớ ko hợp lệ) (vd. nếu SysTick xen
     * vào giữa
     * __enable_irq() và lệnh svc trong ntos_port_start_scheduler()). */
    ntos_port_start_tick();

    ntos_port_start_scheduler();
}

ntos_tick_t ntos_tick_get(void)
{
    return ntos_tick_count;
}

bool ntos_tick_reached(ntos_tick_t now, ntos_tick_t target)
{
    /* Signed subtraction wraparound-safe: dùng với độ lệch <= INT32_MAX tick
     * (xem NTOS_DELAY_MAX_TICKS trong kernel/ntos_task.c). KHÔNG dùng "now >=
     * target" - so sánh đó sẽ sai ngay khi ntos_tick_count wrap qua 0. */
    return (int32_t)(now - target) >= 0;
}

void ntos_tick_handler(void)
{
    size_t i;

    ntos_tick_count++;

    /* Wake delayed task: quét task_table O(n) - task_count nhỏ (<= NTOS_MAX_TASKS)
     * nên không cần delayed list riêng. Task chỉ được wake đúng 1 lần vì lần
     * tick sau block_reason không còn NTOS_BLOCK_DELAY. Wake xong phải chèn
     * lại ready_item vào đúng ready_lists[priority] (lúc BLOCKED nó đã bị
     * ntos_list_remove() trong ntos_delay()).
     *
     * QUAN TRỌNG: điều kiện `block_reason == NTOS_BLOCK_DELAY` là bắt buộc,
     * không được rút gọn về chỉ `state == BLOCKED`. `state == BLOCKED` chỉ
     * cho biết task không runnable, không cho biết task đang chờ hết tick hay
     * chờ semaphore/queue. Một task BLOCKED vì IPC có `wake_tick` còn nguyên
     * giá trị mặc định 0 (chưa từng gọi ntos_delay()), và
     * `ntos_tick_reached(now, 0)` gần như luôn đúng (xem hàm bên dưới - đúng
     * với mọi `now` từ 0 tới gần nửa chu kỳ wraparound). Nếu bỏ điều kiện
     * `block_reason`, tick handler sẽ coi một task đang chờ IPC như đã hết
     * delay ngay tick kế tiếp, đẩy `ready_item` vào `ready_lists[]` trong khi
     * `wait_item` vẫn còn nguyên trong wait list của semaphore/queue - task
     * đó vừa RUNNABLE vừa vẫn đăng ký chờ event, và lần block tiếp theo của
     * nó sẽ thấy `wait_item` chưa được gỡ ra từ lần trước. */
    for (i = 0U; i < task_count; ++i)
    {
        if ((task_table[i]->state == NTOS_TASK_BLOCKED) &&
            (task_table[i]->block_reason == NTOS_BLOCK_DELAY) &&
            ntos_tick_reached(ntos_tick_count, task_table[i]->wake_tick))
        {
            /* Task BLOCKED vì delay không được đồng thời có wait_item còn gắn
             * vào wait list nào (semaphore/queue). Assert lại đúng tại điểm wake để
             * bắt sớm nếu có ai vô tình phá lại bất biến này. */
            NTOS_ASSERT(task_table[i]->wait_item.container == NULL);

            task_table[i]->block_reason = NTOS_BLOCK_NONE;
            task_table[i]->state = NTOS_TASK_READY;
            ntos_list_push_back(&ready_lists[task_table[i]->priority], &task_table[i]->ready_item);

            /* Assert wake xong task phải nằm đúng ready list theo priority của
             * nó - bắt lỗi nếu push_back sai chỗ hoặc priority bị đổi mà không
             * cập nhật ready list tương ứng. */
            NTOS_ASSERT(task_table[i]->ready_item.container ==
                        &ready_lists[task_table[i]->priority]);
        }
    }

    /* Request PendSV vô điều kiện mỗi tick - đủ để xử lý cả 2 trường hợp:
     * round-robin time-slice giữa task cùng priority, VÀ "high-priority task
     * vừa wake preempt low" - ntos_scheduler_select_next() luôn chọn lại task
     * priority cao nhất đang READY, nên không cần thêm logic so sánh priority
     * riêng ở đây. */
    if (ntos_scheduler_running)
    {
        ntos_port_request_context_switch();
    }
}

void ntos_scheduler_register_task(ntos_tcb_t *tcb)
{
    NTOS_ASSERT(tcb != NULL);
    NTOS_ASSERT(task_count < NTOS_MAX_TASKS);

    /* task_table[] vẫn giữ để ntos_tick_handler() quét wake theo wake_tick. */
    task_table[task_count] = tcb;
    task_count++;

    /* Task mới tạo luôn READY - đưa vào đúng ready_lists[priority]. */
    ntos_list_push_back(&ready_lists[tcb->priority], &tcb->ready_item);

    /* Task vừa vào RUNNABLE lần đầu phải thoả đúng invariant READY: nằm đúng
     * ready list theo priority, không mang bất kỳ dấu vết BLOCKED nào (chưa
     * task nào từng block trước khi được đăng ký ở đây, nhưng assert lại cho
     * chắc thay vì ngầm định). */
    NTOS_ASSERT(tcb->ready_item.container == &ready_lists[tcb->priority]);
    NTOS_ASSERT(tcb->block_reason == NTOS_BLOCK_NONE);
    NTOS_ASSERT(tcb->wait_item.container == NULL);
}

void ntos_scheduler_select_next(void)
{
    int p;
    ntos_tcb_t *selected;

    NTOS_ASSERT(ntos_current_tcb != NULL);

    /* Ready list chứa toàn bộ task có thể chạy, gồm cả READY và RUNNING. */
    if (ntos_current_tcb->state == NTOS_TASK_RUNNING)
    {
        ntos_current_tcb->state = NTOS_TASK_READY;
    }

    /* Idle luôn runnable, không bao giờ block, nên ready_lists[0] tuyệt đối
     * không được rỗng - nếu rỗng nghĩa là idle đã bị loại khỏi ready list một
     * cách bất thường (bug nghiêm trọng). */
    NTOS_ASSERT(!ntos_list_is_empty(&ready_lists[0]));

    /* Quét ready_lists[] từ priority cao nhất xuống thấp nhất - NTOS_MAX_PRIORITIES
     * chỉ là 8 nên chưa cần bitmap/CLZ. Không dùng task_table[] để chọn task
     * nữa - chỉ còn dùng cho tick handler wake. */
    selected = NULL;
    for (p = (int)NTOS_MAX_PRIORITIES - 1; p >= 0; --p)
    {
        if (!ntos_list_is_empty(&ready_lists[p]))
        {
            /* ntos_list_next_owner() tự tiến round-robin cursor của riêng
             * list này - chính là round-robin giữa các task cùng priority. */
            selected = (ntos_tcb_t *)ntos_list_next_owner(&ready_lists[p]);
            break;
        }
    }

    /* Idle luôn nằm trong ready_lists[0] và không bao giờ BLOCKED nên
     * ready_lists[] không bao giờ rỗng hết - phải luôn tìm được 1 task. Tại
     * thời điểm này không còn task nào RUNNING nữa (vừa demote ở trên), nên
     * mọi task trong ready_lists[] đều phải là READY. */
    NTOS_ASSERT(selected != NULL);
    NTOS_ASSERT(selected->state == NTOS_TASK_READY);
    NTOS_ASSERT(selected->ready_item.container == &ready_lists[selected->priority]);
    /* RUNNING/READY không được mang theo dấu vết BLOCKED - nếu block_reason
     * còn khác NONE ở đây nghĩa là một đường wake nào đó quên reset field này
     * trước khi đưa task vào ready_lists[]. */
    NTOS_ASSERT(selected->block_reason == NTOS_BLOCK_NONE);
    /* stack_pointer phải luôn nằm trong đúng vùng stack của chính task đó -
     * giống hệt assert đã có ở ntos_start() cho first task, áp dụng lại mỗi
     * lần context switch để phát hiện sớm stack overflow/TCB hỏng thay vì để
     * PendSV restore một context nằm ngoài vùng nhớ hợp lệ. */
    NTOS_ASSERT(selected->stack_pointer >= selected->stack_low);
    NTOS_ASSERT(selected->stack_pointer < selected->stack_high);

    ntos_current_tcb = selected;
    ntos_current_tcb->state = NTOS_TASK_RUNNING;
}
