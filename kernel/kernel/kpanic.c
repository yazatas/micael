#include <kernel/tty.h>
#include <kernel/kprint.h>
#include <kernel/kpanic.h>
#include <stdint.h>

#define REG(reg) reg, reg

void kpanic(const char *error)
{
    asm ("cli");
    tty_init(VGA_COLOR_WHITE, VGA_COLOR_BLUE);
    kdebug("kernel panic: fatal exception: %s!\n", error);

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
    kprint("\teax: 0x%7x %7u\n\tebx: 0x%7x %7u\n\tecx: 0x%7x %7u\n"
           "\tedx: 0x%7x %7u\n\tedi: 0x%7x %7u\n\tcr2: 0x%7x %7u\n",
           "\tcr3: 0x%7x %7u\n", REG(eax), REG(ebx), REG(ecx), 
		   REG(edx), REG(edi), REG(cr2), REG(cr3));

	/* TODO: print backtrace (is it even possible?) */
    
    while (1) { }
    __builtin_unreachable();
}
