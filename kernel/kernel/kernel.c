#include <stdio.h>

#include <limits.h>
#include <stdint.h>

#include <kernel/tty.h>
#include <kernel/gdt.h>
#include <kernel/idt.h>

void kernel_main(void)
{
	gdt_init();
	idt_init();
	term_init();

	int v = 10 / 0;
}
