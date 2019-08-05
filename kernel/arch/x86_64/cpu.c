#include <kernel/cpu.h>
#include <kernel/kprint.h>

#define REG(reg) reg, reg

void arch_dump_registers(void)
{
    uint64_t eax, ebx, ecx, edx, edi, cr3, cr2;

    asm volatile ("mov %%rax, %0\n\
                   mov %%rbx, %1\n\
                   mov %%rcx, %2\n\
                   mov %%rdx, %3\n\
                   mov %%rdi, %4\n\
                   mov %%cr2, %%rax\n\
                   mov %%rax, %5\n\
                   mov %%cr3, %%rax\n\
                   mov %%rax, %6"
                   : "=r"(eax), "=r"(ebx), "=r"(ecx), "=r"(edx),
                     "=r"(edi), "=g"(cr2), "=g"(cr3));

    kprint("\nregister contents:\n");
    kprint("\trax: 0x%09x %10u\n\trbx: 0x%09x %10u\n\trcx: 0x%09x %10u\n"
           "\trdx: 0x%09x %10u\n\trdi: 0x%09x %10u\n\tcr2: 0x%09x %10u\n"
           "\tcr3: 0x%09x %10u\n\n\n", REG(eax), REG(ebx), REG(ecx),
           REG(edx), REG(edi),    REG(cr2), REG(cr3));

}
