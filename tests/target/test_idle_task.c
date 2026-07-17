/*
 * CHANG 6 - Buoc 6C: kiem tra idle task.
 *
 * Giai doan 1 (tu dong, xem qua UART): app_task (priority 1) toggle LED0 va
 * bao cao PASS dinh ky - chung minh yeu cau "co application task priority > 0
 * dang READY thi idle khong choc CPU cua no" (app_task chay deu dan, LED0
 * nhay lien tuc).
 *
 * Giai doan 2 (CAN GDB/OpenOCD, khong tu dong bao PASS/FAIL duoc): sau
 * BLOCK_AFTER_REPORTS lan bao cao, app_task TU dat state = NTOS_TASK_BLOCKED
 * cho chinh no (test-only: ghi truc tiep field cong khai, Chang 6 chua co API
 * block/delay cong khai) roi goi ntos_yield() 1 lan de nhuong CPU ngay lap
 * tuc. Tu thoi diem do, task_table chi con idle dang READY - scheduler BAT
 * BUOC phai chon idle (chung minh 2 yeu cau con lai: "chi co idle runnable ->
 * idle chay" va "application task khong runnable -> idle duoc chon"). Vi idle
 * dung dung spec la vong lap rong (khong tu bao cao duoc), buoc nay phai xac
 * nhan bang debugger:
 *   - LED0 ngung nhay va UART het in bao cao vinh vien.
 *   - Gan GDB, halt, kiem tra ntos_current_tcb == &ntos_idle_task_tcb (extern
 *     trong kernel/internal/ntos_kernel_internal.h) hoac PC dang nam trong
 *     ham ntos_idle_task (kernel/ntos_kernel.c).
 *   - He thong khong hang o NTOS_ASSERT nao (tuc la scheduler khong bao gio
 *     "het task READY" - idle luon la fallback hop le).
 */
#include <ntos/ntos.h>
#include <board.h>
#include <uart.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define STACK_WORDS ((size_t)64U)
#define APP_PRIORITY ((ntos_priority_t)1U)
#define LED_BUSY_WAIT_LOOPS 300000U
#define BLOCK_AFTER_REPORTS 5U

static ntos_tcb_t app_task_tcb;
static _Alignas(8) uint32_t app_task_stack[STACK_WORDS];

/* Debugger/UART doc bien nay de kiem chung app_task duoc chon khi con READY. */
static volatile uint32_t g_app_task_run_count;

static void busy_wait(volatile uint32_t count)
{
    while (count-- > 0U)
    {
        __asm volatile("nop");
    }
}

static void app_task(void *argument)
{
    (void)argument;
    uint32_t report_count = 0U;

    for (;;)
    {
        board_led_toggle(BOARD_LED_D0);
        g_app_task_run_count++;
        busy_wait(LED_BUSY_WAIT_LOOPS);

        bsp_uart_write_string("[6C] app_task_running_idle_not_starving: PASS\r\n");

        report_count++;
        if (report_count >= BLOCK_AFTER_REPORTS)
        {
            bsp_uart_write_string("\r\n[6C] app_task tu block ngay bay gio - tu day idle phai\r\n"
                                   "chay vinh vien. Kiem tra bang GDB: ntos_current_tcb phai\r\n"
                                   "bang &ntos_idle_task_tcb, LED0 ngung nhay, UART het in.\r\n\r\n");

            /* TEST-ONLY: tu block chinh minh - khong co API block/delay cong
             * khai o Chang 6. An toan vi day la state cua CHINH task dang chay,
             * khong phai task khac. */
            app_task_tcb.state = NTOS_TASK_BLOCKED;
            ntos_yield();

            /* Khong bao gio toi day - khong co gi unblock lai app_task. */
            for (;;)
            {
            }
        }
    }
}

int main(void)
{
    ntos_status_t status_app;

    /* board_init() da lam ca GPIO LED lan bsp_uart_init() - khong goi lai o day. */
    board_init();

    /* ntos_init() tao idle task (priority 0) - phai goi truoc khi tao task
     * application de idle da san sang trong task_table khi ntos_start() chay. */
    ntos_init();

    status_app = ntos_task_create_static(&app_task_tcb, "app_task", app_task, NULL,
        app_task_stack, STACK_WORDS, APP_PRIORITY);

    if (status_app != NTOS_OK)
    {
        for (;;)
        {
        }
    }

    ntos_start(&app_task_tcb);

    for (;;)
    {
    }
}
