#include "ntos_assert_internal.h"
void ntos_assert_failed(const char *file, int line, const char *expression)
{
    (void)file; (void)line; (void)expression;
    __asm volatile("cpsid i" ::: "memory");
    for (;;) { __asm volatile("bkpt 0"); }
}
