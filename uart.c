#include "defines.h"
#include "eh2.h"

void UartInit(uint32_t sysclk,uint32_t baudrate)
{

    uint32_t cfg = ((sysclk / 16) / baudrate);


    int clk = SYSCON->CLK_FREQ_HZ;

    UART->LCR = 0x80; /* 當bit7為1, reg00 為設定baudrate, 當bit7為0 reg00為txdata */
    UART->BRDL = (clk / 16 / 115200);
    UART->LCR = (LCR_CS8 | LCR_1_STB | LCR_PDIS);
    UART->FCR = (FCR_FIFO | FCR_MODE0 | FCR_FIFO_8 | FCR_RCVRCLR | FCR_XMITCLR);
    UART->IER = 0;

}


void uart_putc(char c)
{
     while (!(UART->LSR & LSR_THRE))
        ;// asm volatile( "fence iorw,iorw" );

    UART->THR = c;

}

void uart_print(const char *str)
{
    while (*str) {
        uart_putc(*str++);
    }
}