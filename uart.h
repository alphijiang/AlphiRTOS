#ifndef __UART_H__
#define __UART_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

extern void UartInit(uint32_t sysclk,uint32_t baudrate);
extern int uart_putc(char c);
extern void uart_print(const char *str);

#ifdef __cplusplus
}
#endif

#endif /* __UART_H__ */
