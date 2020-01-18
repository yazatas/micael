#ifndef __X86_64_CPU_H__
#define __X86_64_CPU_H__
#ifdef __x86_64__

#include <stdint.h>

typedef struct task task_t;

#define MAX_CPU           64
#define FS_BASE   0xC0000100
#define GS_BASE   0xC0000101
#define KGS_BASE  0xC0000102

typedef struct isr_regs {
    uint64_t eax, ecx, edx, ebx, ebp, esi, edi;
    uint64_t isr_num, err_num;
    uint64_t eip, cs, eflags, esp, ss;
} __attribute__((packed)) isr_regs_t;

static inline void disable_irq(void)
{
    asm volatile ("cli" ::: "memory");
}

static inline void enable_irq(void)
{
    asm volatile ("sti" ::: "memory");
}

static inline uint64_t get_sp(void)
{
    uint64_t sp;
    asm volatile ("mov %%rsp, %0" : "=r"(sp));

    return sp;
}

static inline uint64_t get_msr(uint32_t msr)
{
    uint32_t lo, hi;
    asm volatile ("rdmsr" : "=a" (lo), "=d" (hi) : "c" (msr));

    return ((uint64_t)hi) << 32 | (uint32_t)lo;
}

static inline void set_msr(uint32_t msr, uint64_t reg)
{
    asm volatile ("wrmsr" ::
        "a" (reg & 0xffffffff),
        "d" ((reg >> 32 & 0xffffffff)),
        "c" (msr)
    );
}

static inline void cpu_relax(void)
{
    asm volatile ("pause");
}

void arch_dump_registers(isr_regs_t *cpu_state);
void arch_context_load(unsigned long cr3, void *exec_state);
void arch_context_prepare(task_t *task, void *ip, void *sp);
void arch_context_switch(void **p_kstack, void *c_kstack);
void arch_context_switch_user(void **p_kstack, void *c_estate);

#endif /* __x86_64__ */
#endif /* __X86_64_CPU_H__ */
