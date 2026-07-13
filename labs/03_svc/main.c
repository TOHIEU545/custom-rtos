#include <stdint.h>

typedef struct
{
    uint32_t r0;
    uint32_t r1;
    uint32_t r2;
    uint32_t r3;
    uint32_t r12;
    uint32_t lr;
    uint32_t pc;
    uint32_t xpsr;
} exception_frame_t;

volatile uint32_t g_svc_result;

volatile uint32_t g_test0_return;
volatile uint32_t g_test1_return;

void svc_handler_c(exception_frame_t *frame, uint32_t exc_return);

__attribute__((naked))
void SVC_Handler(void)
{
    __asm volatile(
        "tst lr, #4       \n"
        "ite eq           \n"
        "mrseq r0, msp    \n"
        "mrsne r0, psp    \n"
        "mov r1, lr       \n"
        "b svc_handler_c  \n"
    );
}

__attribute__((noinline))
void svc_handler_c(exception_frame_t *frame, uint32_t exc_return)
{
    const uint8_t *return_address = (const uint8_t *)(uintptr_t)frame->pc;
    /* PC được lưu trong exception frame trỏ tới lệnh ngay sau SVC.
     * SVC là lệnh Thumb 16-bit dài 2 byte, nên lùi 2 byte để đọc SVC number.
     */
    uint8_t svc_number = return_address[-2];

    switch (svc_number)
    {
        case 0:
            g_svc_result = frame->r0;
            break;

        case 1:
            frame->r0 = frame->r0 + frame->r1;
            g_svc_result = frame->r0;
            break;

        default:
            frame->r0 = 0xFFFFFFFFU;
            g_svc_result = frame->r0;
            break;
    }
}

static uint32_t svc_test_r0(void)
{
    __asm volatile(
        "movs r0, #44 \n"
        "svc #0       \n"
    );

    return 0u;
}

static uint32_t svc_add(uint32_t a, uint32_t b)
{
    // Tạo biến tên arg0 và arg1 và yêu cầu compile gắn với thanh ghi r0 và r1
    register uint32_t arg0 __asm("r0") = a;
    register uint32_t arg1 __asm("r1") = b;

    __asm volatile(
        "svc #1"
        : "+r"(arg0)
        : "r"(arg1)
        : "memory"
    );

    return arg0;
}

int main(void)
{
    g_test0_return = svc_test_r0();
    g_test1_return = svc_add(10U, 20U);

    for (;;)
    {
        __asm volatile("nop");
    }
}