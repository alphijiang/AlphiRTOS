#include "rtos.h"

typedef struct {
    uint32_t *sp;       /* 任務堆疊指標 */
    uint32_t stage;     /* 任務生命週期狀態 */
} tcb_t;

volatile tcb_t tasks[MAX_TASKS];
volatile int current_task = 1;
volatile uint32_t task_stacks[MAX_TASKS][STACK_SIZE];

uint32_t enter_critical(void) {
    uint32_t mstatus_val;
    __asm__ volatile ("csrrc %0, mstatus, 0x8" : "=r"(mstatus_val) : : "memory");
    return mstatus_val;
}

void exit_critical(uint32_t mstatus_val) {
    __asm__ volatile ("csrw mstatus, %0" : : "r"(mstatus_val) : "memory");
}

/* 初始化任務堆疊：嚴格對應 kernel.S 的 128 bytes 影格結構 */
void rtos_create_task(int tid, void (*entry)(void)) {
    if (tid < 0 || tid >= MAX_TASKS) return;

    uint32_t *sp = (uint32_t *)&task_stacks[tid][STACK_SIZE];
    *--sp = 0x00001880;                /* mstatus : 修正為 MPP = 11 (M-mode) */
    *--sp = (uint32_t)entry;           /* mepc    第一次 mret 執行的 PC     */
    *--sp = 0;                         /* x31 (t6) */
    *--sp = 0;                         /* x30 (t5) */
    *--sp = 0;                         /* x29 (t4) */
    *--sp = 0;                         /* x28 (t3) */
    *--sp = 0;                         /* x27 (s11) */
    *--sp = 0;                         /* x26 (s10) */
    *--sp = 0;                         /* x25 (s9) */
    *--sp = 0;                         /* x24 (s8) */
    *--sp = 0;                         /* x23 (s7) */
    *--sp = 0;                         /* x22 (s6) */
    *--sp = 0;                         /* x21 (s5) */
    *--sp = 0;                         /* x20 (s4) */
    *--sp = 0;                         /* x19 (s3) */
    *--sp = 0;                         /* x18 (s2) */
    *--sp = 0;                         /* x17 (a7) */
    *--sp = 0;                         /* x16 (a6) */
    *--sp = 0;                         /* x15 (a5) */
    *--sp = 0;                         /* x14 (a4) */
    *--sp = 0;                         /* x13 (a3) */
    *--sp = 0;                         /* x12 (a2) */   
    *--sp = 0;                         /* x11 (a1) */
    *--sp = 0;                         /* x10 (a0) */
    *--sp = 0;                         /* x9 (s1) */
    *--sp = 0;                         /* x8 (s0/fp) */
    *--sp = 0;                         /* x7 (t2) */
    *--sp = 0;                         /* x6 (t1) */
    *--sp = 0;                         /* x5 (t0) */
    *--sp = 0;                         /* x4 (a3) */
    *--sp = 0;                         /* x3 (a2) */
    *--sp = (uint32_t)entry;              /* x1 (ra) */

     tasks[tid].sp = entry ? sp : NULL; /* 如果 entry 為 NULL，則不分配堆疊 */
          
   
    tasks[tid].stage = STAGE_READY;
}

/* C 語言排程器 (對應 kernel.S 傳入的 sp) */
uint32_t schedule(uint32_t sp) {

    
    if (tasks[current_task].stage == STAGE_RUNNING) {
        tasks[current_task].stage = STAGE_READY;
    }

    /* 儲存現任任務的 sp */
    tasks[current_task].sp = (uint32_t *)sp;

    /* 簡單的 Round-Robin 輪替尋找下一個 READY 任務 */
    int next_task = current_task;
    for (int i = 0; i < MAX_TASKS; i++) {
        next_task = (next_task + 1) % MAX_TASKS;
        if (tasks[next_task].stage == STAGE_READY) {
            break;
        }
    }

    current_task = next_task;
    tasks[current_task].stage = STAGE_RUNNING;

    /* 重設 SweRV EH2 MTIMER，維持週期觸發 */
    SYSCON->MTIMECMP = SYSCON->MTIME + (SYSCON->CLK_FREQ_HZ / 100); // 重設計數器初值

    /* 回傳新任務的 sp 給 kernel.S */
    return (uint32_t)tasks[current_task].sp;
}

/* 初始化 EH2 MTIMER */
void rtos_timer_init(void) {
    uint32_t mask = 0;
    uint32_t r = 0;

    // 停用全域中斷 (使用 csrrc 讀取原值並清除 MSTATUS 的 MIE 位元)
    __asm__ volatile ("csrrc %[out], mstatus, %[mask]" : [out]"=r"(r) : [mask]"r"(1 << EH2_MSTATUS_MIE));
    
    write_csr(MCAUSE, 0);


    SYSCON->MTIME = 0; // 設定計數器初值
    SYSCON->MTIMECMP = SYSCON->CLK_FREQ_HZ / 100; // 設定計時器重載值 (以系統頻率的一定比例作為時間片)
    // 啟用  中斷 (使用 csrrs 讀取原值並設定 MIE 的對應 IRQ 位元)
    __asm__ volatile ("csrrs %[out], mie, %[mask]" : [out]"=r"(r) : [mask]"r"(1 << 7));

    // 啟動 MIT0
    //write_csr(MITCTL0, 0x1);

    // 恢復 MSTATUS 中斷致能 (使用 csrrs 讀取原值並設定 MSTATUS 的 MIE 位元)
    __asm__ volatile ("csrrs %[out], mstatus, %[mask]" : [out]"=r"(r) : [mask]"r"(1 << EH2_MSTATUS_MIE));
}

void rtos_init(void) {
    for (int i = 0; i < MAX_TASKS; i++) {
        tasks[i].stage = STAGE_IDLE;
        tasks[i].sp = 0;
    }
}

void rtos_start(void) {
    int first_task = -1;

    /* 1. 動態尋找第一個狀態為 STAGE_READY 的合法任務 */
    for (int i = 0; i < MAX_TASKS; i++) {
        if (tasks[i].stage == STAGE_READY) {
            first_task = i;
            break;
        }
    }

    /* 如果沒有任何可執行的任務，直接進入死循環保護 */
    if (first_task == -1) {
        while (1);
    }

    /* 2. 設定當前任務並改為執行中狀態 */
    current_task = first_task;
    tasks[current_task].stage = STAGE_RUNNING;

    /* 3. 開啟 M-mode 全域中斷 (MIE) */
    uint32_t r = read_csr(MSTATUS);
    r |= (1 << EH2_MSTATUS_MIE);
    write_csr(MSTATUS, r);

    /* 4. 正式交棒：將第一個任務的 sp 傳給組合語言 (放入 a0) */
    rtos_start_asm((uint32_t)tasks[current_task].sp);
}
