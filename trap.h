#ifndef __TRAP_H__
#define __TRAP_H__


#ifdef __cplusplus
extern "C" {
#endif

void __attribute__((aligned(4), interrupt("machine"))) trap_handler(void);

#ifdef __cplusplus
}
#endif

#endif
