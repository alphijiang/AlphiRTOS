#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "defines.h"
#include "eh2.h"
#include "uart.h"
#include "trap.h"
#include "rtos.h"





void task1_entry(void) {
    uint32_t status;
    while(1) {
        status = enter_critical();
        uart_print("Task 1 (ID:1) Running...\n");
        exit_critical(status);
        for(volatile int i=0; i<SYSCON->CLK_FREQ_HZ/100; i++);
    }
}

void task5_entry(void) {
    uint32_t status;
    while(1) {
        status = enter_critical();
        uart_print("Task 5 (ID:5) Running...\n");
        exit_critical(status);
        for(volatile int i=0; i<SYSCON->CLK_FREQ_HZ/100; i++);
    }
}

void task8_entry(void) {
    uint32_t status;
    while(1) {
        status = enter_critical();
        uart_print("Task 8 (ID:8) Running...\n");
        exit_critical(status);
        for(volatile int i=0; i<SYSCON->CLK_FREQ_HZ/100; i++);
    }
}

void task9_entry(void) {
    uint32_t status;
    while(1) {
        status = enter_critical();
        uart_print("Task 9 (ID:9) Running...\n");
        exit_critical(status);
        for(volatile int i=0; i<SYSCON->CLK_FREQ_HZ/100; i++);
    }
}



int main()
{

    // 1. 設定中斷向量表指向 kernel.S 的 trap_entry
    //__asm__ volatile("csrw mtvec, %0" : : "r"(trap_entry));
    UartInit(SYSCON->CLK_FREQ_HZ,115200);

    printf("\nAlphi aRTOS\r\n");


    rtos_init();


    rtos_create_task(1, task1_entry);
    rtos_create_task(5, task5_entry);
    rtos_create_task(8, task8_entry);
    rtos_create_task(9, task9_entry);


    // 3. 初始化 SweRV EH2 專屬硬體計時器 (MIT0)
    // 注意：這個函數內已經包含啟用 MIE 的邏輯
    rtos_timer_init();

    // 4. 呼叫 C 語言的 rtos_start，它會開中斷並跳入 rtos_start_asm
    rtos_start();
   

    
    while (1)
    {
        


    }



    while (1)
        ;

}
