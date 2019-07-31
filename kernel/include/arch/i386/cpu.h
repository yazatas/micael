#ifndef __I386_CPU_H__
#define __I386_CPU_H__
#ifdef __i386__

#include <stdint.h>

typedef struct isr_regs {
    uint16_t gs, fs, es, ds;
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax; /* pusha */
    uint32_t isr_num, err_num;
    uint32_t eip, cs, eflags, useresp, ss; /*  pushed by cpu */
} __attribute__((packed)) isr_regs_t;

static inline void disable_irq(void)
{
    asm volatile ("cli" ::: "memory");
}

static inline void enable_irq(void)
{
    asm volatile ("sti" ::: "memory");
}

static inline uint32_t get_sp(void)
{
    uint32_t sp;
    asm volatile ("mov %%esp, %0" : "=r"(sp));

    return sp;
}

static inline uint64_t get_msr(uint32_t msr, uint32_t *high, uint32_t *low)
{
    uint32_t lo, hi;

    asm volatile ("rdmsr" : "=a" (lo), "=d" (hi) : "c" (msr));

    return ((uint64_t)hi) << 32 | (uint32_t)lo;
}

static inline void set_msr(uint32_t msr, uint64_t high, uint64_t low)
{
    asm volatile ("wrmsr" ::
        "a" (low),
        "d" (high),
        "c" (msr)
    );
}

#endif /* __i386__ */
#endif /* __I386_CPU_H__ */
