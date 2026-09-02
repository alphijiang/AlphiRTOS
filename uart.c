// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 Western Digital Corporation or its affiliates.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

#include <stdarg.h>
#include <stdint.h>
#include <sys/stat.h>
#include "eh2.h"


#define LCR_CS8 0x03   /* 8 bits data size */
#define LCR_1_STB 0x00 /* 1 stop bit */
#define LCR_PDIS 0x00  /* parity disable */

#define LSR_THRE 0x20
#define FCR_FIFO 0x01    /* enable XMIT and RCVR FIFO */
#define FCR_RCVRCLR 0x02 /* clear RCVR FIFO */
#define FCR_XMITCLR 0x04 /* clear XMIT FIFO */
#define FCR_MODE0 0x00 /* set receiver in mode 0 */
#define FCR_MODE1 0x08 /* set receiver in mode 1 */
#define FCR_FIFO_8 0x80  /* 8 bytes in RCVR FIFO */

void UartInit(uint32_t sysclk,uint32_t baudrate)
{

    UART->LCR = 0x80; /* 當bit7為1, reg00 為設定baudrate, 當bit7為0 reg00為txdata */
    UART->DLL = (sysclk / 16 / 115200) & 0xff;
    UART->DLM = ((sysclk / 16 / 115200) >> 8) & 0xff;
    UART->LCR = (LCR_CS8 | LCR_1_STB | LCR_PDIS);
    UART->FCR = (FCR_FIFO | FCR_MODE0 | FCR_FIFO_8 | FCR_RCVRCLR | FCR_XMITCLR);
    UART->IER = 0;


}
int
uart_putc(char c)
{


    while (!(UART->LSR & LSR_THRE))
        ;// asm volatile( "fence iorw,iorw" );

    UART->THR = c;

    return c;
}


void uart_print(const char *str)
{
    while (*str)
    {
        uart_putc(*str++);
    }
}

