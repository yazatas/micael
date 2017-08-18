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
#include <kernel/pfa.h>

void kmain(void)
{
    tty_init_default();
	kprint("hello, world!");

	for (;;);
}
