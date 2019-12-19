#ifndef __CPU_H__
#define __CPU_H__

#ifdef __x86_64__
#   include <arch/x86_64/cpu.h>
#elif defined(__i386__)
#   include <arch/i386/cpu.h>
#else
#warning "architecture not supported!"
#endif

#endif /* __CPU_H__ */
