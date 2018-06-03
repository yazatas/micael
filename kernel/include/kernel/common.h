#ifndef __COMMON_H__
#define __COMMON_H__

#include <stdint.h>
#include <stddef.h>
#include <limits.h>

typedef struct isr_regs {
    uint16_t gs, fs, es, ds;
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax; /* pusha */
    uint32_t isr_num, err_num;
    uint32_t eip, cs, eflags, useresp, ss; /*  pushed by cpu */
} isr_regs_t;

#define MIN(v1, v2) ((v1 < v2) ? v1 : v2)
#define MAX(v1, v2) ((v1 < v2) ? v2 : v1)
#define ROUND_DOWN(addr, boundary) (addr & ~(boundary - 1))
#define ROUND_UP(addr,   boundary) ((addr % boundary) ? \
								   ((addr & ~(boundary - 1)) + boundary) : \
									(addr))

#endif /* end of include guard: __COMMON_H__ */
