#include <kernel/arch.h>
#include <arch/x86_64/gdt.h>

extern unsigned long gdt_ptr;

void arch_init(void)
{
    gdt_init();

    /* TODO: initialize tss */
    /* TODO: initialize idt */
    /* TODO: initialize hardware */
}
