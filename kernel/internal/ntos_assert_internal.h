#ifndef NTOS_ASSERT_INTERNAL_H
#define NTOS_ASSERT_INTERNAL_H
void ntos_assert_failed(const char *file, int line, const char *expression);
#define NTOS_ASSERT(expr) do { if (!(expr)) ntos_assert_failed(__FILE__, __LINE__, #expr); } while (0)
#endif
