#ifndef __RTOS_H__
#define __RTOS_H__

#include <stddef.h>
#include "eh2.h"

#define MAX_TASKS       10 /* TASK 最多執行數量 */ 
#define STACK_SIZE      256

#define STAGE_IDLE      0
#define STAGE_READY     1
#define STAGE_SLEEP     2
#define STAGE_RUNNING   3

#ifdef __cplusplus
extern "C" {
#endif

extern void rtos_init(void);
extern void rtos_create_task(int tid, void (*entry)(void));
extern void rtos_start(void);
extern uint32_t schedule(uint32_t sp);
extern void rtos_timer_init(void);
extern void rtos_start_asm(uint32_t initial_sp);
extern uint32_t enter_critical(void);
extern void exit_critical(uint32_t state);

#ifdef __cplusplus
}
#endif

#endif /* __RTOS_H__ */
