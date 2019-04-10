#ifndef __ARCH_X86_64_GDT_H__
#define __ARCH_X86_64_GDT_H__

#include <arch/x86_64/gdtdef.h>
#include <kernel/gdt.h>

/* struct tss_ptr_t { */
/* 	unsigned prev_tss; /1* for hw task switching, not supported *1/ */
/*   	unsigned esp0;     /1* The stack pointer to load when we change to kernel mode *1/ */
/*   	unsigned ss0;      /1* The stack segment to load when we change to kernel mode *1/ */

/*   	/1* unused *1/ */
/*   	unsigned esp1, ss1, esp2, ss2, cr3, eip, eflags; */
/*   	unsigned eax,  ecx, edx,  ebx, esp, ebp, esi, edi, trap; */
/*   	unsigned es,   cs,  ss,   ds,  fs,  gs,  ldt, iomap_base; */
/* } __attribute__((packed)); */

#endif /* __ARCH_X86_64_GDT_H__ */
