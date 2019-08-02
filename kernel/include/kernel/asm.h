#ifndef __ASM_H__
#define __ASM_H__

#ifdef __x86_64__
#   include <arch/x86_64/asm.h>
#elif defined(__i386__)
#   include <arch/i386/asm.h>
#else
#warning "architecture not supported!"
#endif

#endif /* __ASM_H__ */
