#include "ipc_demo.h"

#include <ntos/ntos.h>
#include <board.h>

int main(void)
{
    board_init();
    ntos_init();

    ipc_demo_init();
    ipc_demo_start();

    for (;;)
    {
    }
}
