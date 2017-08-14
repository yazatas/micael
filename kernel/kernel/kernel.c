#include <stdio.h>
#include <limits.h>
#include <stdint.h>
#include <string.h>

#include <kernel/tty.h>
#include <kernel/gdt.h>
#include <kernel/idt.h>
#include <kernel/irq.h>
#include <kernel/timer.h>
#include <kernel/keyboard.h>
#include <kernel/kprint.h>

#include <kernel/mmu.h>
#include <kernel/pfa.h>

void kernel_main(void)
{
    term_init_default();
    gdt_init();
    idt_init();
    irq_init();
    asm ("sti"); /* enable hardware interrupts */
    /* mmu_init(); */

    timer_install();
    kb_install();

    uint32_t *ptr;
    extern uint32_t __kernel_end;
    pageframe_t tmp = kalloc_frame();

    tmp = kalloc_frame();
    ptr = (uint32_t*)tmp;

    kprint("kernel end: 0x%x\n"
           "page start: 0x%x\n"
           "ptr start:  0x%x\n",
           &__kernel_end, tmp, ptr);

    memset(ptr, 'q', PF_SIZE);

    for (int i = 0; i < 30; ++i) {
        term_putc(*(ptr + i));
    }
}
