#ifndef __EH2_H__
#define __EH2_H__

#include <stdint.h>
#include "defines.h"

#define __IM const volatile
#define __OM volatile
#define __IOM volatile

#define HW64_REG(x) *((volatile unsigned long long *)(x))
#define HW32_REG(x) *((volatile unsigned long int *)(x))
#define HW16_REG(x) *((volatile unsigned short int *)(x))
#define HW8_REG(x) *((volatile unsigned char *)(x))

#define __STR(s)                #s
#define STRINGIFY(s)            __STR(s)

#define __RV_CSR_READ(csr)                                      \
    ({                                                          \
        register unsigned int __v;                              \
        asm volatile("csrr %0, " STRINGIFY(csr)                 \
                     : "=r"(__v)                                \
                     :                                          \
                     : "memory");                               \
        __v;                                                    \
    })

#define __RV_CSR_WRITE(csr, val)                                \
    ({                                                          \
        register unsigned int __v = (unsigned int)(val);        \
        asm volatile("csrw " STRINGIFY(csr) ", %0"            \
                     :                                          \
                     : "rK"(__v)                                \
                     : "memory");                               \
    })


#define read_csr __RV_CSR_READ
#define write_csr __RV_CSR_WRITE

#define SYSCON_BASE     0x80001000
#define SPI_BASE        0x80001040
#define UART_BASE       0x80002000

#define __IM const volatile
#define __OM volatile
#define __IOM volatile

#define BAUD_RATE 115200

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



typedef struct SysConTypedef
{
  volatile unsigned char VERSION_PATCH;     /* 0x00 VeeRwolf patch version */
  volatile unsigned char VERSION_MINOR;     /* 0x01 VeeRwolf minor version */
  volatile unsigned char VERSION_MAJOR;     /* 0x02 VeeRwolf major version */
  volatile unsigned char VERSION_MISC;      /* 0x03 VeeRwolf patch version */
  volatile unsigned int  VERSION_SHA;       /* 0x04~0x07 VeeRwolf patch version */
  volatile unsigned char SIM_PRINT;         /* 0x08 VeeRwolf patch version */
  volatile unsigned char SIM_EXIT;          /* 0x09  	*/
  volatile unsigned char INT_STATUS;        /* 0x0A Bit 0 = RAM initialization complete. Bit 1 = RAM initialization reported errors */
  volatile unsigned char SW_IRQ;            /* 0x0B */
  volatile unsigned int  NMI_VEC;           /* 0x0C~0x0F Interrupt vector for NMI */
  volatile unsigned int GPIO0;             /* 0x10~0x13 32 readable and writable GPIO bits */
  volatile unsigned int :32;                /* 0x14~0x17 */
  volatile unsigned int GPIO1;             /* 0x18~0x1B 32 readable and writable GPIO bits */
  volatile unsigned int :32;                /* 0x1C~0x19 */
  volatile unsigned long long int MTIME;    /* 0x20~0x27 mtime from RISC-V privilege spec */
  volatile unsigned long long int MTIMECMP; /* 0x28~0x2f mtimecmp from RISC-V privilege spec */
  volatile unsigned int IRQ_TIMER_CNT;      /* 0x30~0x33 IRQ timer counter */
  volatile unsigned int IRQ_TIMER_CTRL : 8; /* 0x34 IRQ timer control */
  volatile unsigned int :24;                /* 0x35~0x37 */
  volatile unsigned int :32;                /* 0x38~0x3B*/
  volatile unsigned int CLK_FREQ_HZ;        /* 0x3C ~0x3F Clock frequency of main clock in Hz */

}SysConTypedef;

/* =========================================================================================================================== */
/* ================                                           SPI                                             ================ */
/* =========================================================================================================================== */


/**
  * @brief Serial Peripheral Interface (SPI)
  */

typedef struct {                                /*!< (@ 0x80001800) SPI0 Structure                                             */

  union {
    __IOM uint32_t SPCR;                        /*!< (@ 0x00000000) Control Register                                           */

    struct {
      __IOM uint32_t SPR        : 2;            /*!< [1..0] These bits select the SPI clock rate. Refer to the ESPR
                                                     bits in the Extension Register for more information.                      */
      __IOM uint32_t CPHA       : 1;            /*!< [2..2] Clock Phase.
                                                     0: SCK is low when idle. The leading edge of a clock cycle
                                                     is a rising edge, while the trailing edge is a falling
                                                     edge.
                                                     1: SCK is high when idle. The leading edge of a clock cycle
                                                     is a falling edge, while the trailing edge is a rising
                                                     edge.
                                                                                                                               */
      __IOM uint32_t CPOL       : 1;            /*!< [3..3] Clock Polarity
                                                     0: CLK high sample.
                                                     1: CLK low sample.
                                                                                                                               */
      __IOM uint32_t MSTR       : 1;            /*!< [4..4] Master Mode Select
                                                     0: Slave mode.
                                                     1: Master mode.
                                                                                                                               */
            uint32_t            : 1;
      __IOM uint32_t SPE        : 1;            /*!< [6..6] Serial Peripheral Enable.
                                                     0: SPI core disabled
                                                     1: SPI core enabled                                                       */
      __IOM uint32_t SPIE       : 1;            /*!< [7..7] Serial Peripheral Interrupt Enable
                                                     0: SPI interrupts disable
                                                     1: SPI interrupts enable
                                                                                                                               */
            uint32_t            : 24;
    } SPCR_b;
  } ;
  __IM  uint32_t  :32;

  union {
    __IOM uint32_t SPSR;                        /*!< (@ 0x00000008) Serial Peripheral Status Register                          */

    struct {
      __IM  uint32_t RFEMPTY    : 1;            /*!< [0..0] The Read FIFO Full and Read FIFO empty bits show the
                                                     status of the read FIFO.                                                  */
      __IM  uint32_t RFFULL     : 1;            /*!< [1..1] The Read FIFO Full and Read FIFO empty bits show the
                                                     status of the read FIFO.                                                  */
      __IM  uint32_t WFEMPTY    : 1;            /*!< [2..2] The Write FIFO Full and Write FIFO empty bits show the
                                                     status of the write FIFO.                                                 */
      __IM  uint32_t WFFULL     : 1;            /*!< [3..3] The Write FIFO Full and Write FIFO empty bits show the
                                                     status of the write FIFO.                                                 */
            uint32_t            : 2;
      __IOM uint32_t WCOL       : 1;            /*!< [6..6] The Write Collision flag is set when the Serial Peripheral
                                                     Data register is written to,
                                                     while the Write FIFO is full. To clear the Write Collision
                                                     flag write the status register
                                                     with the WCOL bit set (?1?).                                          */
      __IOM uint32_t SPIF       : 1;            /*!< [7..7] The Serial Peripheral Interrupt Flag is set upon completion
                                                     of a transfer block. If SPIF
                                                     is asserted (1) and SPIE is set, an interrupt is generated.
                                                     To clear the interrupt write
                                                     the status register with the SPIF bit set (1)                             */
            uint32_t            : 24;
    } SPSR_b;
  } ;
  __IM  uint32_t  :32;
  __IOM uint32_t  SPDR;                         /*!< (@ 0x00000010) Serial Peripheral Data Register                            */
  __IM  uint32_t  :32;

  union {
    __IOM uint32_t SPER;                        /*!< (@ 0x00000018) Serial Peripheral Extensions Register                      */

    struct {
      __IOM uint32_t ESPR       : 2;            /*!< [1..0] ESPR SPR Divide clock
                                                     00 00 2
                                                     00 01 4
                                                     00 10 16
                                                     00 11 32
                                                     01 00 8
                                                     01 01 64
                                                     01 10 128
                                                     01 11 256
                                                     10 00 512
                                                     10 01 1024
                                                     10 10 2048
                                                     10 11 4096
                                                                                                                               */
            uint32_t            : 4;
      __IOM uint32_t ICNT       : 2;            /*!< [7..6] Interrupt Count
                                                     00: SPIF is set after every completed transfer
                                                     01: SPIF is set after every two completed transfers
                                                     10: SPIF is set after every three completed transfers
                                                     11: SPIF is set after every four completed transfers                      */
            uint32_t            : 24;
    } SPER_b;
  } ;
  __IM  uint32_t  :32;
  __IOM uint32_t  SPSS;                         /*!< (@ 0x00000020) SPI slave select.
                                                                    1: SS pin is active
                                                                    0: SS pin is inactive                                      */
} SPI_Type;                                     /*!< Size = 36 (0x24)                                                          */



typedef struct UartTypedef
{
  union
  {
    volatile unsigned int BRDL; /* 0x00 Baud rate divisor (LSB)        */
    volatile unsigned int THR;  /* 0x00 Transmit and Hold             */
  };
  
  volatile unsigned int IER;    /* 0x04 Interrupt enable reg.          */
  volatile unsigned int FCR;    /* 0x08 FIFO control reg.              */
  volatile unsigned int LCR;    /* 0x0C Line control reg.              */
  volatile unsigned int :32;
  volatile unsigned int LSR;    /* 0x14 Line status reg.               */

} UartTypedef;



#define SYSCON  ((SysConTypedef *)SYSCON_BASE)
#define SPI     ((SPI_Type *)SPI_BASE)
#define UART    ((UartTypedef *)UART_BASE)

#define EH2_MSTATUS_SIE		0
#define EH2_MSTATUS_UIE		1
#define EH2_MSTATUS_MIE		3

#define EH2_TIMER0_IRQ_NUM 29
#define EH2_TIMER1_IRQ_NUM 28



#endif
