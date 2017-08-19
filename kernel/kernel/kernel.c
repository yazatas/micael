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
#include <kernel/kpanic.h>
#include <kernel/mmu.h>

void kmain(void)
{
    tty_init_default();
	gdt_init();
	idt_init();
	irq_init();
	asm ("sti"); /* enable interrupts */

	timer_install();
	kb_install();

	kprint("haloust");
	for (;;);
}
