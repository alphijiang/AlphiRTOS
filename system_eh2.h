#ifndef __SYSTEM_EH2_H__
#define __SYSTEM_EH2_H__
#ifdef __cplusplus
extern "C" {
#endif  
__attribute__((always_inline)) static inline void set_csr_bit(unsigned int csr_address, unsigned int bit)
{
	asm volatile("csrrs t0,%0,%1\n"::"i"(csr_address),"r"(1<<bit):"t0");
}
__attribute__((always_inline)) static inline void clr_csr_bit(unsigned int csr_address, unsigned int bit)
{
	asm volatile("csrrc t0,%0,%1\n"::"i"(csr_address),"r"(1<<bit):"t0");
}

__attribute__((always_inline)) static inline void disable_ext_interrupt(void)
{
    asm volatile("li t0,0x800\n"
                 "csrc mie,t0\n"
    			 "fence.i":::"t0");
}

__attribute__((always_inline)) static inline  void enable_ext_interrupt(void)
{
    asm volatile("li t0,0x800\n"
                 "csrs mie,t0\n"
    			 "fence.i":::"t0");
}

__attribute__((always_inline)) static inline void init_pic_priorityorder(int priord)
{
        __RV_WRITE_MEM((RV_PIC_BASE_ADDR + RV_PIC_MPICCFG_OFFSET), priord);
}

__attribute__((always_inline)) static inline void init_pic_nstthresholds(int threshold)
{
        write_csr(MEICIDPL, threshold);
        write_csr(MEICURPL, threshold);
        asm volatile("fence.i");
}

__attribute__((always_inline)) static inline void set_pic_threshold(int threshold)
{
        __RV_WRITE_MEM((RV_PIC_BASE_ADDR + RV_PIC_MEIPT_OFFSET), threshold);
}

__attribute__((always_inline)) static inline void enable_pic_interrupt(int id)
{
        __RV_WRITE_MEM(((RV_PIC_BASE_ADDR + RV_PIC_MEIE_OFFSET) + (id << 2)), 1);
}

__attribute__((always_inline)) static inline void disable_pic_interrupt(int id)
{
        __RV_WRITE_MEM(((RV_PIC_BASE_ADDR + RV_PIC_MEIE_OFFSET) + (id << 2)), 0);
}

__attribute__((always_inline)) static inline void set_pic_priority(int id, int priority)
{
        __RV_WRITE_MEM(((RV_PIC_BASE_ADDR + RV_PIC_MEIPL_OFFSET) + (id << 2)), priority);
}

__attribute__((always_inline)) static inline void init_pic_gateway(int id, int polarity, int type)
{
        __RV_WRITE_MEM(((RV_PIC_BASE_ADDR + RV_PIC_MEIGWCTRL_OFFSET) + (id << 2)), ((type << 1) | polarity));
}

__attribute__((always_inline)) static inline void clear_pic_gateway(int id)
{
        __RV_WRITE_MEM(((RV_PIC_BASE_ADDR + RV_PIC_MEIGWCLR_OFFSET) + (id << 2)), 0);
}


#ifdef __cplusplus
}
#endif
#endif  /*__SYSTEM_EH2_H__*/