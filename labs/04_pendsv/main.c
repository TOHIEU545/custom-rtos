#include <board.h>
#include <uart.h>
int main(void) { bsp_board_init(); bsp_uart_write_string("lab_pendsv\r\n"); for (;;) {} }
