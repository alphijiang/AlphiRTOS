#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "defines.h"
#include "eh2.h"

#ifndef __SEMIHOSTING

int _write (int file, char *ptr, int len)
{

    int i;

    /* Turn character to capital letter and output to UART port */
    for (i = 0; i < len; i++)
    {
        while (!(UART->LSR & LSR_THRE))
            ;

        UART->THR = *ptr++;
        //whisperPutc(((int)*ptr++));
    }

    return len;
}
// 標準輸入 `_read` 的實現
int _read(int file, char *ptr, int len)
{
   return 0;
}
#endif

// `_sbrk` 的實現，用於提供堆的記憶體分配
void *_sbrk(ptrdiff_t incr)
{
    extern char _end; // 堆的起始地址，由鏈接腳本提供
    extern char _heap_end; // 堆的結束地址，由鏈接腳本提供
    static char *heap_ptr = &_end; // 初始化堆指針
    char *prev_heap_ptr = heap_ptr;

    if ((heap_ptr + incr) > &_heap_end)
    {
        // 如果超過堆的範圍，返回錯誤
        return (void *) -1;
    }

    heap_ptr += incr;
    return prev_heap_ptr;
}

// `_close` 的實現
int _close(int file)
{
    (void)file;
    return 0;
}

// `_lseek` 的實現
int _lseek(int file, int ptr, int dir)
{
    (void)file;
    (void)ptr;
    (void)dir;
    return 0;
}

// `_isatty` 的實現
int _isatty(int file)
{
    (void)file;
    return 1;
}

int _fstat (int file, struct stat *st)
{
    return 0;
}

// _kill: 處理終止進程的系統調用
int _kill(int pid, int sig)
{
    (void)pid; // 忽略 pid
    (void)sig; // 忽略 sig
    // 嵌入式系統通常不支持進程管理，直接返回錯誤
    return -1; // 返回 -1 表示失敗
}

// _getpid: 獲取當前進程 ID
int _getpid(void)
{
    return 1; // 返回固定的進程 ID（嵌入式系統通常只有一個進程）
}
