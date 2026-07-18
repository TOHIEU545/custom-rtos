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

/* Ready list theo tung priority (Chang 8) - dai dien toan bo task RUNNABLE
 * (RUNNING + READY). BLOCKED/UNUSED khong duoc nam trong day. index cua tung
 * list tu lam round-robin cursor rieng cho priority do (thay cho
 * current_task_index kieu cu quet task_table). Khong con "static" vi export
 * qua kernel_internal.h chi de test kiem tra invariant container. */
ntos_list_t ready_lists[NTOS_MAX_PRIORITIES];

ntos_tcb_t ntos_idle_task_tcb;
static _Alignas(8) uint32_t ntos_idle_task_stack[NTOS_IDLE_STACK_WORDS];

/* Idle task Chang 6: khong lam gi ca, khong block, khong goi semaphore/queue.
 * Priority 0 (thap nhat) - chi duoc chon khi khong con task application nao
 * priority cao hon dang READY (xem quy uoc round-robin cung priority da
 * quyet dinh trong ntos_scheduler_select_next()). */
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

    /* Phai init xong het ready_lists[] TRUOC khi tao task dau tien (ke ca idle) -
     * ntos_scheduler_register_task() se ntos_list_push_back() vao day ngay. */
    for (p = 0U; p < NTOS_MAX_PRIORITIES; ++p)
    {
        ntos_list_init(&ready_lists[p]);
    }

    status = ntos_task_create_static(&ntos_idle_task_tcb, "idle", ntos_idle_task,
        NULL, ntos_idle_task_stack, NTOS_IDLE_STACK_WORDS, 0U);

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
    /* Chang 8: thay vi do task_table[] tim index, kiem tra thang first_task
     * dang nam dung ready_lists[priority] cua no - dung invariant moi
     * (RUNNABLE task phai co ready_item.container tro dung list). */
    NTOS_ASSERT(first_task_tcb->ready_item.container == &ready_lists[first_task_tcb->priority]);

    first_task_tcb->state = NTOS_TASK_RUNNING;
    ntos_current_tcb = first_task_tcb;

    /* Priority PendSV/SysTick va SysTick timer phai san sang truoc khi bat interrupt.
     * ntos_scheduler_running KHONG duoc dat true o day - SVC_Handler (port) tu dat
     * true ngay sau khi PSP cua first task hop le, de loai bo hoan toan kha nang
     * SysTick pend PendSV truoc khi PSP san sang (vd. neu SysTick xen vao giua
     * __enable_irq() va lenh svc trong ntos_port_start_scheduler()). */
    ntos_port_start_tick();

    ntos_port_start_scheduler();
}

ntos_tick_t ntos_tick_get(void)
{
    return ntos_tick_count;
}

bool ntos_tick_reached(ntos_tick_t now, ntos_tick_t target)
{
    /* Signed subtraction wraparound-safe: dung voi do lech <= INT32_MAX tick
     * (xem NTOS_DELAY_MAX_TICKS trong kernel/ntos_task.c). KHONG dung "now >=
     * target" - so sanh do se sai ngay khi ntos_tick_count wrap qua 0. */
    return (int32_t)(now - target) >= 0;
}

void ntos_tick_handler(void)
{
    size_t i;

    ntos_tick_count++;

    /* Wake delayed task (Chang 7): quet task_table O(n) - task_count nho
     * (<= NTOS_MAX_TASKS) nen khong can delayed list rieng o giai doan nay.
     * Task chi duoc wake dung 1 lan vi lan tick sau state khong con BLOCKED.
     * Chang 8: wake xong phai chen lai ready_item vao dung ready_lists[priority]
     * (luc BLOCKED no da bi ntos_list_remove() trong ntos_delay()). */
    for (i = 0U; i < task_count; ++i)
    {
        if ((task_table[i]->state == NTOS_TASK_BLOCKED) &&
            ntos_tick_reached(ntos_tick_count, task_table[i]->wake_tick))
        {
            task_table[i]->state = NTOS_TASK_READY;
            ntos_list_push_back(&ready_lists[task_table[i]->priority], &task_table[i]->ready_item);

            /* Debug check (muc 11): wake xong task phai nam dung ready list
             * theo priority cua no - bat loi neu push_back sai cho hoac priority
             * bi doi ma khong cap nhat ready list tuong ung. */
            NTOS_ASSERT(task_table[i]->ready_item.container == &ready_lists[task_table[i]->priority]);
        }
    }

    /* Request PendSV vo dieu kien moi tick (giu nguyen tu Chang 5) - da du de
     * xu ly ca 2 truong hop: round-robin time-slice giua task cung priority,
     * VA "high-priority task vua wake preempt low" - ntos_scheduler_select_next()
     * (Chang 6) luon chon lai task priority cao nhat dang READY, nen khong can
     * them logic so sanh priority rieng o day. */
    if (ntos_scheduler_running)
    {
        ntos_port_request_context_switch();
    }
}

void ntos_scheduler_register_task(ntos_tcb_t *tcb)
{
    NTOS_ASSERT(tcb != NULL);
    NTOS_ASSERT(task_count < NTOS_MAX_TASKS);

    /* task_table[] van giu de ntos_tick_handler() quet wake theo wake_tick
     * (Chang 7, chua toi uu thanh delayed list rieng o Chang 8). */
    task_table[task_count] = tcb;
    task_count++;

    /* Task moi tao luon READY - dua vao dung ready_lists[priority] (Chang 8). */
    ntos_list_push_back(&ready_lists[tcb->priority], &tcb->ready_item);
}

void ntos_scheduler_select_next(void)
{
    int p;
    ntos_tcb_t *selected;

    NTOS_ASSERT(ntos_current_tcb != NULL);

    /* RUNNING -> READY nhung KHONG remove khoi ready list: invariant Chang 8
     * la ready_lists[] dai dien RUNNING + READY gop lai, chi BLOCKED/UNUSED
     * moi khong nam trong day. */
    if (ntos_current_tcb->state == NTOS_TASK_RUNNING)
    {
        ntos_current_tcb->state = NTOS_TASK_READY;
    }

    /* Debug check (muc 11): idle luon runnable, khong bao gio block, nen
     * ready_lists[0] tuyet doi khong duoc rong - neu rong nghia la idle da
     * bi loai khoi ready list mot cach bat thuong (bug nghiem trong). */
    NTOS_ASSERT(!ntos_list_is_empty(&ready_lists[0]));

    /* Quet ready_lists[] tu priority cao nhat xuong thap nhat - NTOS_MAX_PRIORITIES
     * chi la 8 nen chua can bitmap/CLZ (Chang 8 khong lam). Khong con dung
     * task_table[] de chon task nua - chi con dung cho tick handler wake (Chang 7). */
    selected = NULL;
    for (p = (int)NTOS_MAX_PRIORITIES - 1; p >= 0; --p)
    {
        if (!ntos_list_is_empty(&ready_lists[p]))
        {
            /* ntos_list_next_owner() tu tien round-robin cursor cua rieng
             * list nay - chinh la round-robin giua cac task cung priority. */
            selected = (ntos_tcb_t *)ntos_list_next_owner(&ready_lists[p]);
            break;
        }
    }

    /* Idle luon nam trong ready_lists[0] va khong bao gio BLOCKED nen
     * ready_lists[] khong bao gio rong het - phai luon tim duoc 1 task. Tai
     * thoi diem nay khong con task nao RUNNING nua (vua demote o tren), nen
     * moi task trong ready_lists[] deu phai la READY. */
    NTOS_ASSERT(selected != NULL);
    NTOS_ASSERT(selected->state == NTOS_TASK_READY);
    NTOS_ASSERT(selected->ready_item.container == &ready_lists[selected->priority]);

    ntos_current_tcb = selected;
    ntos_current_tcb->state = NTOS_TASK_RUNNING;
}
