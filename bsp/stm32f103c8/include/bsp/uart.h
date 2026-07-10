#ifndef BSP_UART_H
#define BSP_UART_H
#include <stddef.h>
#include <stdint.h>
void bsp_uart_init(void);
void bsp_uart_write_byte(uint8_t byte);
void bsp_uart_write(const void *data, size_t size);
void bsp_uart_write_string(const char *string);
#endif
