/*
 * STM32F411xE reset sequence and vector table.
 *
 * The vector order and handler names follow STMicroelectronics' official
 * startup_stm32f411xe.s from cmsis-device-f4 commit
 * 9192c7b9df75a142f2027ab266601fe061fc00b3. This project keeps an equivalent
 * C implementation so its reset flow is visible alongside the other board
 * support code.
 */
#include <stdint.h>

typedef void (*isr_handler_t)(void);

extern uint32_t _estack;
extern uint32_t _sidata;
extern uint32_t _sdata;
extern uint32_t _edata;
extern uint32_t _sbss;
extern uint32_t _ebss;

void SystemInit(void);
int main(void);

void Reset_Handler(void);
void Default_Handler(void);

#define WEAK_DEFAULT_HANDLER(name) void name(void) __attribute__((weak, alias("Default_Handler")))

WEAK_DEFAULT_HANDLER(NMI_Handler);
WEAK_DEFAULT_HANDLER(HardFault_Handler);
WEAK_DEFAULT_HANDLER(MemManage_Handler);
WEAK_DEFAULT_HANDLER(BusFault_Handler);
WEAK_DEFAULT_HANDLER(UsageFault_Handler);
WEAK_DEFAULT_HANDLER(SVC_Handler);
WEAK_DEFAULT_HANDLER(DebugMon_Handler);
WEAK_DEFAULT_HANDLER(PendSV_Handler);
WEAK_DEFAULT_HANDLER(SysTick_Handler);

WEAK_DEFAULT_HANDLER(WWDG_IRQHandler);
WEAK_DEFAULT_HANDLER(PVD_IRQHandler);
WEAK_DEFAULT_HANDLER(TAMP_STAMP_IRQHandler);
WEAK_DEFAULT_HANDLER(RTC_WKUP_IRQHandler);
WEAK_DEFAULT_HANDLER(FLASH_IRQHandler);
WEAK_DEFAULT_HANDLER(RCC_IRQHandler);
WEAK_DEFAULT_HANDLER(EXTI0_IRQHandler);
WEAK_DEFAULT_HANDLER(EXTI1_IRQHandler);
WEAK_DEFAULT_HANDLER(EXTI2_IRQHandler);
WEAK_DEFAULT_HANDLER(EXTI3_IRQHandler);
WEAK_DEFAULT_HANDLER(EXTI4_IRQHandler);
WEAK_DEFAULT_HANDLER(DMA1_Stream0_IRQHandler);
WEAK_DEFAULT_HANDLER(DMA1_Stream1_IRQHandler);
WEAK_DEFAULT_HANDLER(DMA1_Stream2_IRQHandler);
WEAK_DEFAULT_HANDLER(DMA1_Stream3_IRQHandler);
WEAK_DEFAULT_HANDLER(DMA1_Stream4_IRQHandler);
WEAK_DEFAULT_HANDLER(DMA1_Stream5_IRQHandler);
WEAK_DEFAULT_HANDLER(DMA1_Stream6_IRQHandler);
WEAK_DEFAULT_HANDLER(ADC_IRQHandler);
WEAK_DEFAULT_HANDLER(EXTI9_5_IRQHandler);
WEAK_DEFAULT_HANDLER(TIM1_BRK_TIM9_IRQHandler);
WEAK_DEFAULT_HANDLER(TIM1_UP_TIM10_IRQHandler);
WEAK_DEFAULT_HANDLER(TIM1_TRG_COM_TIM11_IRQHandler);
WEAK_DEFAULT_HANDLER(TIM1_CC_IRQHandler);
WEAK_DEFAULT_HANDLER(TIM2_IRQHandler);
WEAK_DEFAULT_HANDLER(TIM3_IRQHandler);
WEAK_DEFAULT_HANDLER(TIM4_IRQHandler);
WEAK_DEFAULT_HANDLER(I2C1_EV_IRQHandler);
WEAK_DEFAULT_HANDLER(I2C1_ER_IRQHandler);
WEAK_DEFAULT_HANDLER(I2C2_EV_IRQHandler);
WEAK_DEFAULT_HANDLER(I2C2_ER_IRQHandler);
WEAK_DEFAULT_HANDLER(SPI1_IRQHandler);
WEAK_DEFAULT_HANDLER(SPI2_IRQHandler);
WEAK_DEFAULT_HANDLER(USART1_IRQHandler);
WEAK_DEFAULT_HANDLER(USART2_IRQHandler);
WEAK_DEFAULT_HANDLER(EXTI15_10_IRQHandler);
WEAK_DEFAULT_HANDLER(RTC_Alarm_IRQHandler);
WEAK_DEFAULT_HANDLER(OTG_FS_WKUP_IRQHandler);
WEAK_DEFAULT_HANDLER(DMA1_Stream7_IRQHandler);
WEAK_DEFAULT_HANDLER(SDIO_IRQHandler);
WEAK_DEFAULT_HANDLER(TIM5_IRQHandler);
WEAK_DEFAULT_HANDLER(SPI3_IRQHandler);
WEAK_DEFAULT_HANDLER(DMA2_Stream0_IRQHandler);
WEAK_DEFAULT_HANDLER(DMA2_Stream1_IRQHandler);
WEAK_DEFAULT_HANDLER(DMA2_Stream2_IRQHandler);
WEAK_DEFAULT_HANDLER(DMA2_Stream3_IRQHandler);
WEAK_DEFAULT_HANDLER(DMA2_Stream4_IRQHandler);
WEAK_DEFAULT_HANDLER(OTG_FS_IRQHandler);
WEAK_DEFAULT_HANDLER(DMA2_Stream5_IRQHandler);
WEAK_DEFAULT_HANDLER(DMA2_Stream6_IRQHandler);
WEAK_DEFAULT_HANDLER(DMA2_Stream7_IRQHandler);
WEAK_DEFAULT_HANDLER(USART6_IRQHandler);
WEAK_DEFAULT_HANDLER(I2C3_EV_IRQHandler);
WEAK_DEFAULT_HANDLER(I2C3_ER_IRQHandler);
WEAK_DEFAULT_HANDLER(FPU_IRQHandler);
WEAK_DEFAULT_HANDLER(SPI4_IRQHandler);
WEAK_DEFAULT_HANDLER(SPI5_IRQHandler);

__attribute__((used, section(".isr_vector")))
const isr_handler_t g_pfnVectors[] = {
    (isr_handler_t)&_estack,
    Reset_Handler,
    NMI_Handler,
    HardFault_Handler,
    MemManage_Handler,
    BusFault_Handler,
    UsageFault_Handler,
    0,
    0,
    0,
    0,
    SVC_Handler,
    DebugMon_Handler,
    0,
    PendSV_Handler,
    SysTick_Handler,

    WWDG_IRQHandler,
    PVD_IRQHandler,
    TAMP_STAMP_IRQHandler,
    RTC_WKUP_IRQHandler,
    FLASH_IRQHandler,
    RCC_IRQHandler,
    EXTI0_IRQHandler,
    EXTI1_IRQHandler,
    EXTI2_IRQHandler,
    EXTI3_IRQHandler,
    EXTI4_IRQHandler,
    DMA1_Stream0_IRQHandler,
    DMA1_Stream1_IRQHandler,
    DMA1_Stream2_IRQHandler,
    DMA1_Stream3_IRQHandler,
    DMA1_Stream4_IRQHandler,
    DMA1_Stream5_IRQHandler,
    DMA1_Stream6_IRQHandler,
    ADC_IRQHandler,
    0,
    0,
    0,
    0,
    EXTI9_5_IRQHandler,
    TIM1_BRK_TIM9_IRQHandler,
    TIM1_UP_TIM10_IRQHandler,
    TIM1_TRG_COM_TIM11_IRQHandler,
    TIM1_CC_IRQHandler,
    TIM2_IRQHandler,
    TIM3_IRQHandler,
    TIM4_IRQHandler,
    I2C1_EV_IRQHandler,
    I2C1_ER_IRQHandler,
    I2C2_EV_IRQHandler,
    I2C2_ER_IRQHandler,
    SPI1_IRQHandler,
    SPI2_IRQHandler,
    USART1_IRQHandler,
    USART2_IRQHandler,
    0,
    EXTI15_10_IRQHandler,
    RTC_Alarm_IRQHandler,
    OTG_FS_WKUP_IRQHandler,
    0,
    0,
    0,
    0,
    DMA1_Stream7_IRQHandler,
    0,
    SDIO_IRQHandler,
    TIM5_IRQHandler,
    SPI3_IRQHandler,
    0,
    0,
    0,
    0,
    DMA2_Stream0_IRQHandler,
    DMA2_Stream1_IRQHandler,
    DMA2_Stream2_IRQHandler,
    DMA2_Stream3_IRQHandler,
    DMA2_Stream4_IRQHandler,
    0,
    0,
    0,
    0,
    0,
    0,
    OTG_FS_IRQHandler,
    DMA2_Stream5_IRQHandler,
    DMA2_Stream6_IRQHandler,
    DMA2_Stream7_IRQHandler,
    USART6_IRQHandler,
    I2C3_EV_IRQHandler,
    I2C3_ER_IRQHandler,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    FPU_IRQHandler,
    0,
    0,
    SPI4_IRQHandler,
    SPI5_IRQHandler,
};

_Static_assert((sizeof(g_pfnVectors) / sizeof(g_pfnVectors[0])) == 102U,
               "STM32F411xE vector table must contain 16 core vectors and 86 IRQ vectors");

void Reset_Handler(void)
{
    const uint32_t *source;
    uint32_t *destination;

    /* Keep the same ordering as ST's official startup: initialize the core
     * first, then initialize the C data regions before entering main(). */
    SystemInit();

    source = &_sidata;
    destination = &_sdata;
    while (destination < &_edata)
    {
        *destination++ = *source++;
    }

    destination = &_sbss;
    while (destination < &_ebss)
    {
        *destination++ = 0U;
    }

    (void)main();

    for (;;)
    {
    }
}

void Default_Handler(void)
{
    for (;;)
    {
    }
}
