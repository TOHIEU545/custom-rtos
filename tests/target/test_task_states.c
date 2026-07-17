/*
 * CHANG 6 - Buoc 6A: kiem tra task state model (READY/RUNNING/BLOCKED/UNUSED)
 * truoc khi lam priority scheduler (6B) va idle task (6C).
 *
 * TEST-ONLY: include thang kernel/internal de doc ntos_current_tcb va dat
 * state BLOCKED/UNUSED truc tiep len TCB (dung ngoai le NTOS_CODING_CONVENTION.md
 * muc 2.4 - Chang 6 chua co API cong khai de block/unblock task).
 */
#include <ntos/ntos.h>
#include <board.h>
#include <uart.h>
#include "ntos_kernel_internal.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define STACK_WORDS ((size_t)64U)
#define REPORT_BUSY_WAIT_LOOPS 1500000U

static ntos_tcb_t task_ready_tcb;
static ntos_tcb_t task_blocked_tcb;
static ntos_tcb_t task_unused_tcb;
static ntos_tcb_t task_reporter_tcb;
static _Alignas(8) uint32_t task_ready_stack[STACK_WORDS];
static _Alignas(8) uint32_t task_blocked_stack[STACK_WORDS];
static _Alignas(8) uint32_t task_unused_stack[STACK_WORDS];
static _Alignas(8) uint32_t task_reporter_stack[STACK_WORDS];

/* Debugger/UART doc cac bien nay de kiem chung scheduler chon/bo qua dung task. */
static volatile uint32_t g_task_ready_run_count;
static volatile uint32_t g_task_blocked_run_count;
static volatile uint32_t g_task_unused_run_count;

/* TEST-ONLY: busy loop lam cham nhip in UART, khong dung SysTick/kernel tick. */
static void busy_wait(volatile uint32_t count)
{
    while (count-- > 0U)
    {
        __asm volatile("nop");
    }
}

static void uart_write_pass_fail(const char *name, bool ok)
{
    bsp_uart_write_string("[6A] ");
    bsp_uart_write_string(name);
    bsp_uart_write_string(ok ? ": PASS\r\n" : ": FAIL\r\n");
}

/* READY task binh thuong - scheduler phai chon chay task nay. */
static void task_ready_entry(void *argument)
{
    (void)argument;

    for (;;)
    {
        g_task_ready_run_count++;
    }
}

/* Bi ep state = BLOCKED ngay sau khi tao, truoc ntos_start() - khong bao gio
 * duoc scheduler chon; neu counter nay tang tuc la scheduler co bug. */
static void task_blocked_entry(void *argument)
{
    (void)argument;

    for (;;)
    {
        g_task_blocked_run_count++;
    }
}

/* Bi ep state = UNUSED ngay sau khi tao, truoc ntos_start() - khong bao gio
 * duoc scheduler chon; neu counter nay tang tuc la scheduler co bug. */
static void task_unused_entry(void *argument)
{
    (void)argument;

    for (;;)
    {
        g_task_unused_run_count++;
    }
}

/* Task nay tu kiem tra invariant "current task luon RUNNING" ngay tren chinh
 * no, roi in ket qua PASS/FAIL cua tat ca kiem tra qua UART theo chu ky. */
static void task_reporter_entry(void *argument)
{
    (void)argument;

    for (;;)
    {
        bool ready_selected = (g_task_ready_run_count > 0U);
        bool blocked_skipped = (g_task_blocked_run_count == 0U);
        bool unused_skipped = (g_task_unused_run_count == 0U);
        bool current_is_running = (ntos_current_tcb == &task_reporter_tcb) &&
                                   (ntos_current_tcb->state == NTOS_TASK_RUNNING);
        bool ready_state_valid = (task_ready_tcb.state == NTOS_TASK_READY) ||
                                  (task_ready_tcb.state == NTOS_TASK_RUNNING);

        uart_write_pass_fail("task_ready_selected", ready_selected);
        uart_write_pass_fail("task_blocked_skipped", blocked_skipped);
        uart_write_pass_fail("task_unused_skipped", unused_skipped);
        uart_write_pass_fail("current_is_running", current_is_running);
        uart_write_pass_fail("task_ready_state_valid", ready_state_valid);
        bsp_uart_write_string("\r\n");

        busy_wait(REPORT_BUSY_WAIT_LOOPS);
    }
}

int main(void)
{
    ntos_status_t status_ready;
    ntos_status_t status_blocked;
    ntos_status_t status_unused;
    ntos_status_t status_reporter;

    board_init();
    ntos_init();

    status_ready = ntos_task_create_static(&task_ready_tcb, "task_ready", task_ready_entry,
        NULL, task_ready_stack, STACK_WORDS, 0U);
    status_blocked = ntos_task_create_static(&task_blocked_tcb, "task_blocked", task_blocked_entry,
        NULL, task_blocked_stack, STACK_WORDS, 0U);
    status_unused = ntos_task_create_static(&task_unused_tcb, "task_unused", task_unused_entry,
        NULL, task_unused_stack, STACK_WORDS, 0U);
    status_reporter = ntos_task_create_static(&task_reporter_tcb, "task_reporter", task_reporter_entry,
        NULL, task_reporter_stack, STACK_WORDS, 0U);

    if ((status_ready != NTOS_OK) || (status_blocked != NTOS_OK) ||
        (status_unused != NTOS_OK) || (status_reporter != NTOS_OK))
    {
        for (;;)
        {
        }
    }

    /* TEST-ONLY: ep state ngay sau khi tao, truoc khi scheduler chay - day la
     * cach duy nhat (chua co API block/unblock cong khai o Chang 6) de kiem tra
     * scheduler bo qua dung task BLOCKED/UNUSED. */
    task_blocked_tcb.state = NTOS_TASK_BLOCKED;
    task_unused_tcb.state = NTOS_TASK_UNUSED;

    ntos_start(&task_ready_tcb);

    for (;;)
    {
    }
}
