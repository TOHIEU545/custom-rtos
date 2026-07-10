#include <bsp/board.h>
#include <ntos/ntos.h>
int main(void) { bsp_board_init(); ntos_init(); /* TODO. */ ntos_start(); }
