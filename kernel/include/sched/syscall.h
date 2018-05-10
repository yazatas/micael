#ifndef __SYSCALL_H__
#define __SYSCALL_H__

#include <kernel/common.h>

void syscall_handler(isr_regs_t *cpu);

#endif /* end of include guard: __SYSCALL_H__ */
