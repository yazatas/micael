#ifndef __COMMON_H__
#define __COMMON_H__

#include <stdint.h>
#include <stddef.h>
#include <limits.h>
#include <kernel/kprint.h>
#include <sys/types.h>

typedef struct isr_regs {
#ifdef __x86_64__
    uint64_t eax, ecx, edx, ebx, ebp, esi, edi;
    uint64_t isr_num, err_num;
    uint64_t eip, cs, eflags, esp, ss;
#else
    uint16_t gs, fs, es, ds;
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax; /* pusha */
    uint32_t isr_num, err_num;
    uint32_t eip, cs, eflags, useresp, ss; /*  pushed by cpu */
#endif
} __attribute__((packed)) isr_regs_t;

#define MIN(v1, v2) (((v1) < (v2)) ? (v1) : (v2))
#define MAX(v1, v2) (((v1) < (v2)) ? (v2) : (v1))

#define ROUND_DOWN(addr, boundary) ((addr) & ~((boundary) - 1))
#define ROUND_UP(addr,   boundary) (((addr) % (boundary)) ? \
                                   (((addr) & ~((boundary) - 1)) + (boundary)) : (addr))
#define MULTIPLE_OF_2(value)       ((value + 1) & -2)
#define ALIGNED(value, aligment)   ((value & (aligment - 1)) == 0)
#define PAGE_ALIGNED(value)        ALIGNED(value, 4096)

static inline void hex_dump(void *buf, size_t len)
{
    for (size_t i = 0; i < len; i+=10) {
        kprint("\t");
        for (size_t k = i; k < i + 10 && k < len; ++k) {
            kprint("0x%02x ", ((uint8_t *)buf)[k]);
        }
        kprint("\n");
    }
}

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

static inline uint64_t read_64(void *ptr)
{
    return *((uint64_t volatile *)ptr);
}

static inline uint32_t read_32(void *ptr)
{
    return *((uint32_t volatile *)ptr);
}

static inline uint16_t read_16(void *ptr)
{
    return *((uint16_t volatile *)ptr);
}

static inline void write_64(void *ptr, uint64_t value)
{
    *((uint64_t volatile *)ptr) = value;
}

static inline void write_32(void *ptr, uint64_t value)
{
    *((uint32_t volatile *)ptr) = value;
}

static inline void write_16(void *ptr, uint64_t value)
{
    *((uint16_t volatile *)ptr) = value;
}

#endif /* end of include guard: __COMMON_H__ */
