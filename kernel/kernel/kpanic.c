#include <stdint.h>

#include <kernel/kprint.h>
#include <kernel/kpanic.h>
#include <kernel/common.h>
#include <mm/mmu.h>

#define REG(reg) reg, reg

struct regs_t {
	uint16_t gs, fs, es, ds;
	uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax; /* pusha */
	uint32_t isr_num, err_num;
	uint32_t eip, cs, eflags, useresp, ss; /*  pushed by cpu */
};

void __attribute__((noreturn))
kpanic(const char *error)
{
    disable_irq();

	struct regs_t *tmp = (struct regs_t*)error;

	kdebug("error number: %d %x", tmp->err_num, tmp->err_num);
	kdebug("'%s'", error);

    uint32_t eax, ebx, ecx, edx, edi, cr3, cr2;

    asm volatile ("mov %%eax, %0\n\
                   mov %%ebx, %1\n\
                   mov %%ecx, %2\n\
                   mov %%edx, %3\n\
                   mov %%edi, %4\n\
                   mov %%cr2, %%eax\n\
                   mov %%eax, %5\n\
                   mov %%cr3, %%eax\n\
                   mov %%eax, %6"
                   : "=r"(eax), "=r"(ebx), "=r"(ecx), "=r"(edx), 
                     "=r"(edi), "=g"(cr2), "=g"(cr3));

    kprint("\nregister contents:\n");
    kprint("\teax: 0x%08x %8u\n\tebx: 0x%08x %8u\n\tecx: 0x%08x %8u\n"
           "\tedx: 0x%08x %8u\n\tedi: 0x%08x %8u\n\tcr2: 0x%08x %8u\n"
           "\tcr3: 0x%08x %8u\n\n\n", REG(eax), REG(ebx), REG(ecx), 
		   REG(edx), REG(edi),    REG(cr2), REG(cr3));

    mmu_print_memory_map();
	mmu_list_pde();

    while (1) { }
    __builtin_unreachable();
}
