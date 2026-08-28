# ------------------------------------------------------------------
# 檔案名稱：trap.S
# 描述：嚴格對齊 128 bytes 影格、無冗餘 sp 儲存的上下文常式
# ------------------------------------------------------------------

.global trap_entry
.global rtos_start_asm
.extern current_task
.extern tasks
.extern schedule

# ------------------------------------------------------------------
# 中斷向量入口
# ------------------------------------------------------------------
trap_entry:

# ------------------------------------------------------------------
# 1. 保存上下文模組 (Save Context)
# ------------------------------------------------------------------
save_context:
    # 步驟 1：向下配置 128 bytes 堆疊空間 (128 = 16 * 8，完美 16-byte 對齊)
    addi sp, sp, -128
    
    # 步驟 2：儲存 30 個通用暫存器 (x1, x3-x31)
    sw x1,   0(sp)  
    sw x3,   4(sp)
    sw x4,   8(sp)
    sw x5,  12(sp)
    sw x6,  16(sp)
    sw x7,  20(sp)
    sw x8,  24(sp)
    sw x9,  28(sp)
    sw x10, 32(sp)
    sw x11, 36(sp)
    sw x12, 40(sp)
    sw x13, 44(sp)
    sw x14, 48(sp)
    sw x15, 52(sp)
    sw x16, 56(sp)
    sw x17, 60(sp)
    sw x18, 64(sp)
    sw x19, 68(sp)
    sw x20, 72(sp)
    sw x21, 76(sp)
    sw x22, 80(sp)
    sw x23, 84(sp)
    sw x24, 88(sp)
    sw x25, 92(sp)
    sw x26, 96(sp)
    sw x27, 100(sp)
    sw x28, 104(sp)
    sw x29, 108(sp)
    sw x30, 112(sp)
    sw x31, 116(sp)
    
    # 步驟 3：儲存 mepc 與 mstatus 系統暫存器 (對應 offset 120 與 124)
    csrr x6, mepc
    sw x6,  120(sp)
    csrr x7, mstatus
    sw x7,  124(sp)

    # 步驟 4：將目前的堆疊指標 sp 存回當前任務的 TCB 中
    #la t3, current_task
    #lw t4, 0(t3)
    #la t5, tasks
    #slli t6, t4, 3        # sizeof(tcb_t) = 8 bytes
    #add t5, t5, t6
    #sw sp, 0(t5)          # tasks[current_task].sp = sp

    # 步驟 5：呼叫 C 語言排程器 (目前任務的 sp 會回傳至 a0)
    mv a0, sp
    jal ra, schedule

    # 步驟 6：切換至新任務的堆疊指標
    mv sp, a0

    # 步驟 7：進入恢復上下文模組
    j restore_context

# ------------------------------------------------------------------
# 2. 恢復上下文模組 (Restore Context)
# ------------------------------------------------------------------
restore_context:
    # 步驟 1：恢復系統狀態暫存器
    lw t0, 124(sp)
    csrw mstatus, t0
    lw t1, 120(sp)
    csrw mepc, t1
    
    # 步驟 2：恢復 30 個通用暫存器
    lw x1,   0(sp)
    lw x3,   4(sp)
    lw x4,   8(sp)
    lw x5,  12(sp)
    lw x6,  16(sp)
    lw x7,  20(sp)
    lw x8,  24(sp)
    lw x9,  28(sp)
    lw x10, 32(sp)
    lw x11, 36(sp)
    lw x12, 40(sp)
    lw x13, 44(sp)
    lw x14, 48(sp)
    lw x15, 52(sp)
    lw x16, 56(sp)
    lw x17, 60(sp)
    lw x18, 64(sp)
    lw x19, 68(sp)
    lw x20, 72(sp)
    lw x21, 76(sp)
    lw x22, 80(sp)
    lw x23, 84(sp)
    lw x24, 88(sp)
    lw x25, 92(sp)
    lw x26, 96(sp)
    lw x27, 100(sp)
    lw x28, 104(sp)
    lw x29, 108(sp)
    lw x30, 112(sp)
    lw x31, 116(sp)
    
    # 步驟 3：釋放 128 bytes 堆疊影格（數學完美對稱還原 sp）
    addi sp, sp, 128
    
    # 步驟 4：硬體返回，正式切換至新任務執行
    mret

# ------------------------------------------------------------------
# RTOS 啟動常式
# ------------------------------------------------------------------
rtos_start_asm:
    # 1. 讀取當前任務 ID (通常是 0)
    la t0, current_task
    lw t4, 0(t0)

    # 2. 計算該任務 TCB 的記憶體位址 (Task ID * 8)
    la t5, tasks
    slli t6, t4, 3
    add t5, t5, t6

    # 3. 載入該任務的 sp
    lw sp, 0(t5)

    # 4. 直接的還原流程，讓它把偽造的 Stack Frame 彈出並 mret
    j restore_context