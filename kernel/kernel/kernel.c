#include <stdio.h>
#include <limits.h>
#include <stdint.h>

#include <kernel/tty.h>
#include <kernel/gdt.h>
#include <kernel/idt.h>
#include <kernel/irq.h>
#include <kernel/timer.h>
#include <kernel/keyboard.h>

void kernel_main(void)
{
	term_init_default();
	gdt_init();
	idt_init();
	irq_init();
	asm ("sti"); /* enable hardware interrupts */

	timer_install();
	kb_install();

	int v = 10 / 0;

	for (;;);
}
