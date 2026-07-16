#ifndef BSP_UART_H
#define BSP_UART_H
/**
 * @file uart.h
 * @brief Driver USART1 polling (PA9=TX, PA10=RX, không remap): init, gửi byte/buffer/string.
 */
#include <stddef.h>
#include <stdint.h>

/**
 * @brief Init USART1: 8 bit, không parity, 1 stop bit, TX+RX enable.
 * @param baudrate Tốc độ baud mong muốn, ví dụ 115200U.
 */
void bsp_uart_init(uint32_t baudrate);

/** @brief Gửi 1 byte, polling chờ cờ TXE. */
void bsp_uart_write_byte(uint8_t byte);

/** @brief Gửi buffer, polling từng byte. */
void bsp_uart_write(const void *data, size_t size);

/** @brief Gửi C-string (không gửi ký tự NUL), polling. */
void bsp_uart_write_string(const char *string);
#endif
