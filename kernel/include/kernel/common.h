#ifndef __COMMON_H__
#define __COMMON_H__

#include <stdint.h>
#include <stddef.h>
#include <limits.h>
#include <kernel/kprint.h>

typedef struct isr_regs {
    uint16_t gs, fs, es, ds;
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax; /* pusha */
    uint32_t isr_num, err_num;
    uint32_t eip, cs, eflags, useresp, ss; /*  pushed by cpu */
} isr_regs_t;

#define MIN(v1, v2) (((v1) < (v2)) ? (v1) : (v2))
#define MAX(v1, v2) (((v1) < (v2)) ? (v2) : (v1))

#define ROUND_DOWN(addr, boundary) (addr & ~(boundary - 1))
#define ROUND_UP(addr,   boundary) ((addr % boundary) ? \
								   ((addr & ~(boundary - 1)) + boundary) : (addr))

static inline void hex_dump(void *buf, size_t len)
{
    for (size_t i = 0; i < len; i+=10) {
        kprint("\t");
        for (size_t k = i; k < i + 10; ++k) {
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

#endif /* end of include guard: __COMMON_H__ */
