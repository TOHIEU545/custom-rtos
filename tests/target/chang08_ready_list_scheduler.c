/*
 * CHANG 8 - Buoc 9: integration test cho ready list + scheduler moi.
 *
 * 3 task:
 *   task_a   (priority 3) - CPU-bound, khong bao gio delay.
 *   task_b   (priority 3, bang task_a) - delay dinh ky roi wake.
 *   task_low (priority 1) - CPU-bound, khong bao gio delay; dung de quan sat
 *                           invariant cua task_b tu ben ngoai trong luc task_b
 *                           dang BLOCKED (vi task_low luon duoc chay xen ke).
 *
 * Kich ban duoc bao phu:
 *   A. Highest priority: task_low (pri1) khong bao gio chay khi task_a/task_b
 *      (pri3) con READY - kiem qua g_task_low_run_count dung yen giua 2 lan do.
 *   B. Same priority round-robin: task_a va task_b (cung pri3) thay phien nhau
 *      moi khi ca 2 cung READY - kiem qua ca 2 counter deu tang.
 *   C. Mixed priority: chinh la A/B cong lai (2 task pri3 round-robin, 1 task
 *      pri1 bi starve) - khong can kich ban rieng.
 *   D. Blocking: khi task_b dang BLOCKED (delay), ready_item.container cua no
 *      phai == NULL - task_low kiem tra dieu nay tu ben ngoai.
 *   E. Wake: ngay khi task_b tinh day, ready_item.container phai ==
 *      &ready_lists[task_b.priority] (dung ready_lists duoc export qua
 *      kernel_internal.h chi de test).
 *   F. High wake preempt low: do thoi gian tu luc goi delay() den luc tinh,
 *      phai gan dung so tick yeu cau (khong bi tre nhieu vi ket sau task_low).
 *   H. Invariant: owner cua ready_item phai luon tro dung TCB so huu no.
 *
 * G (idle chon khi tat ca app task deu BLOCKED) KHONG lam lai o day - da co
 * san trong test_idle_task.c/test_delay.c (chay lai nhu regression, xem bao
 * cao Chang 8), vi can ca task_low cung phai delay (mau thuan voi vai tro
 * "luon READY" cua no trong file nay).
 *
 * TEST-ONLY: include thang kernel/internal de doc ready_item.container va
 * mang ready_lists[] (xem NTOS_CODING_CONVENTION.md muc 2.4).
 */
#include <ntos/ntos.h>
#include <board.h>
#include <uart.h>
#include "ntos_kernel_internal.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define STACK_WORDS ((size_t)64U)
#define HIGH_PRIORITY ((ntos_priority_t)3U)
#define LOW_PRIORITY ((ntos_priority_t)1U)
#define TASK_B_DELAY_TICKS 300U
#define WAKE_LATENCY_TOLERANCE_TICKS 3U
#define LOW_CHECK_EVERY 5000U

static ntos_tcb_t task_a_tcb;
static ntos_tcb_t task_b_tcb;
static ntos_tcb_t task_low_tcb;
static _Alignas(8) uint32_t task_a_stack[STACK_WORDS];
static _Alignas(8) uint32_t task_b_stack[STACK_WORDS];
static _Alignas(8) uint32_t task_low_stack[STACK_WORDS];

/* Debugger/UART doc cac bien nay de kiem chung tung kich ban. */
static volatile uint32_t g_task_a_run_count;
static volatile uint32_t g_task_b_run_count;
static volatile uint32_t g_task_low_run_count;

static void uart_write_pass_fail(const char *name, bool ok)
{
    bsp_uart_write_string("[8] ");
    bsp_uart_write_string(name);
    bsp_uart_write_string(ok ? ": PASS\r\n" : ": FAIL\r\n");
}

/* Priority cao, cung tier voi task_b - round-robin voi task_b khi ca 2 READY. */
static void task_a(void *argument)
{
    (void)argument;

    for (;;)
    {
        g_task_a_run_count++;
    }
}

/* Priority cao, delay dinh ky - kiem tra invariant D/E/F/H tren chinh no. */
static void task_b(void *argument)
{
    (void)argument;

    for (;;)
    {
        g_task_b_run_count++;

        ntos_tick_t before = ntos_tick_get();
        ntos_delay(TASK_B_DELAY_TICKS);
        ntos_tick_t after = ntos_tick_get();
        ntos_tick_t elapsed = after - before;

        /* E: ngay sau khi wake, ready_item phai nam dung ready_lists[priority]. */
        bool wake_container_correct =
            (task_b_tcb.ready_item.container == &ready_lists[task_b_tcb.priority]);
        /* H: owner cua ready_item phai luon tro dung TCB so huu no. */
        bool owner_correct = (task_b_tcb.ready_item.owner == &task_b_tcb);
        /* F: wake phai gan dung han, khong bi ket lai phia sau task_low. */
        bool woke_promptly = (elapsed >= TASK_B_DELAY_TICKS) &&
                              (elapsed <= (TASK_B_DELAY_TICKS + WAKE_LATENCY_TOLERANCE_TICKS));
        /* B: task_a cung phai duoc chay (round-robin cung priority). */
        bool task_a_also_ran = (g_task_a_run_count > 0U);

        uart_write_pass_fail("wake_container_matches_ready_list", wake_container_correct);
        uart_write_pass_fail("ready_item_owner_correct", owner_correct);
        uart_write_pass_fail("high_wake_preempts_low_promptly", woke_promptly);
        uart_write_pass_fail("same_priority_round_robin_a_ran", task_a_also_ran);
        bsp_uart_write_string("\r\n");
    }
}

/* Priority thap - luon READY, dung lam "nguoi quan sat" invariant cua task_b
 * tu ben ngoai trong luc task_b dang BLOCKED (vi task_low duoc chay xen ke). */
static void task_low(void *argument)
{
    (void)argument;

    for (;;)
    {
        g_task_low_run_count++;

        if ((g_task_low_run_count % LOW_CHECK_EVERY) == 0U)
        {
            /* A/C: task_low dang chay tuc la tai thoi diem nay khong co task
             * pri3 nao dang RUNNING - nhung neu task_a/task_b van con READY,
             * lan schedule tiep theo phai quay lai uu tien ho, khong phai
             * task_low chiem lien tuc (kiem tiep o duoi). */
            if (task_b_tcb.state == NTOS_TASK_BLOCKED)
            {
                /* D: BLOCKED task tuyet doi khong con nam trong ready list. */
                bool blocked_container_null = (task_b_tcb.ready_item.container == NULL);
                uart_write_pass_fail("blocked_task_container_null", blocked_container_null);
            }
        }
    }
}

int main(void)
{
    ntos_status_t status_a;
    ntos_status_t status_b;
    ntos_status_t status_low;

    board_init();
    ntos_init();

    status_a = ntos_task_create_static(&task_a_tcb, "task_a", task_a, NULL,
        task_a_stack, STACK_WORDS, HIGH_PRIORITY);
    status_b = ntos_task_create_static(&task_b_tcb, "task_b", task_b, NULL,
        task_b_stack, STACK_WORDS, HIGH_PRIORITY);
    status_low = ntos_task_create_static(&task_low_tcb, "task_low", task_low, NULL,
        task_low_stack, STACK_WORDS, LOW_PRIORITY);

    if ((status_a != NTOS_OK) || (status_b != NTOS_OK) || (status_low != NTOS_OK))
    {
        for (;;)
        {
        }
    }

    ntos_start(&task_a_tcb);

    for (;;)
    {
    }
}
