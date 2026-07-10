#ifndef NTOS_TEST_H
#define NTOS_TEST_H
#define NTOS_TEST_ASSERT(x) do { if (!(x)) for (;;) {} } while (0)
#endif
