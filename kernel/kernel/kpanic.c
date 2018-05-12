#include <kernel/tty.h>
#include <kernel/kprint.h>
#include <kernel/kpanic.h>
#include <stdint.h>

#include <mm/vmm.h>

#define REG(reg) reg, reg

struct regs_t {
	uint16_t gs, fs, es, ds;
	uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax; /* pusha */
	uint32_t isr_num, err_num;
	uint32_t eip, cs, eflags, useresp, ss; /*  pushed by cpu */
};

void kpanic(const char *error)
{
    asm ("cli");
    tty_init(VGA_COLOR_WHITE, VGA_COLOR_BLUE);
    /* kdebug("kernel panic: fatal exception: %s!\n", error); */

	struct regs_t *tmp = (struct regs_t*)error;

	kdebug("error number: %d %x", tmp->err_num, tmp->err_num);
	kdebug("'%s'", error);
	kdebug("%u free pages", vmm_free_pages());

    uint32_t eax, ebx, ecx, edx, edi, eip, cr3, cr2;

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

    kprint("register contents:\n");
    kprint("\teax: 0x%08x %8u\n\tebx: 0x%08x %8u\n\tecx: 0x%08x %8u\n"
           "\tedx: 0x%08x %8u\n\tedi: 0x%08x %8u\n\tcr2: 0x%08x %8u\n"
           "\tcr3: 0x%08x %8u\n", REG(eax), REG(ebx), REG(ecx), 
		   REG(edx), REG(edi),    REG(cr2), REG(cr3));

	/* TODO: print backtrace (is it even possible?) */
	kprint("\n\n0x%x\n", 0);

	/* vmm_list_pde(NULL); */

	/* uint32_t *v = (uint32_t)0x0; */
	/* for (size_t i = 0; i < 10; ++i) */
	/* 	kprint("0x%x ", v[i]); */
    
    while (1) { }
    __builtin_unreachable();
}
