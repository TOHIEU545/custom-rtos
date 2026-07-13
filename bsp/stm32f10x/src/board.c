#include <board.h>
#include <clock.h>
#include <exti.h>
#include <gpio.h>
#include <uart.h>
void bsp_board_init(void) { bsp_clock_init(); bsp_gpio_init(); bsp_uart_init(); bsp_exti_init(); }
uint32_t bsp_core_clock_hz(void) { extern uint32_t SystemCoreClock; return SystemCoreClock; }
