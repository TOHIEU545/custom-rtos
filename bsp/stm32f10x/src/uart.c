#include <uart.h>
void bsp_uart_init(void) { /* TODO. */ }
void bsp_uart_write_byte(uint8_t byte) { (void)byte; /* TODO. */ }
void bsp_uart_write(const void *data, size_t size) { const uint8_t *p=data; for (size_t i=0;i<size;i++) bsp_uart_write_byte(p[i]); }
void bsp_uart_write_string(const char *s) { if (!s) return; while (*s) bsp_uart_write_byte((uint8_t)*s++); }
