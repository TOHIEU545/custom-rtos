#include "scheduler_demo.h"

#include <ntos/ntos.h>
#include <board.h>

int main(void)
{
    board_init();
    ntos_init();

    scheduler_demo_init();
    scheduler_demo_start();

    for (;;)
    {
    }
}
