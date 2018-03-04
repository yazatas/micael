#include <stdio.h>
#include <limits.h>
#include <stdint.h>
#include <string.h>

#include <kernel/tty.h>
#include <kernel/gdt.h>
#include <kernel/idt.h>
#include <kernel/irq.h>
#include <kernel/mmu.h>
#include <kernel/kprint.h>
#include <kernel/kpanic.h>
#include <kernel/task.h>

#include <drivers/timer.h>
#include <drivers/keyboard.h>

extern uint32_t __kernel_virtual_start,  __kernel_virtual_end;
extern uint32_t __kernel_physical_start, __kernel_physical_end;
extern uint32_t boot_page_dir; 

void func_1(void);
void func_2(void);
void func_3(void);

void kmain(void)
{
    tty_init_default();

	kdebug("kvirtual kphysical start: 0x%08x 0x%08x\n"
		   "[kmain] kvirtual kphysical end:   0x%08x 0x%08x",
		   &__kernel_virtual_start, &__kernel_physical_start,
		   &__kernel_virtual_end,   &__kernel_physical_end);
	kdebug("kpage dir start addr:     0x%08x", &boot_page_dir);

	gdt_init(); idt_init();
	irq_init(); timer_install();
	kb_install();
	asm ("sti"); /* enable interrupts */
	mmu_init();

	create_task(func_3, 256, "func_3");
	create_task(func_2, 256, "func_2");
	create_task(func_1, 256, "func_1");
	start_tasking();

	/* NOTE: execution should never get here
	 * if even one of the tasks has an infinite loop */
	kdebug("After start_tasking");

	for (;;);
}

void func_1(void)
{
	kdebug("started func_1");

	while (1) {
		kprint("in function 1\n");
		for (volatile int i = 0; i < 60000000; ++i) {};
		yield();
	}
}

void func_2(void)
{
	kdebug("started func_2");

	for (int i = 0; i < 3; ++i) {
		kprint("in function 2\n");
		for (volatile int i = 0; i < 60000000; ++i) {};
		yield();
	}

	delete_task();
}

void func_3(void)
{
	kdebug("started func_3");

	while (1) {
		kprint("in function 3\n");
		for (volatile int i = 0; i < 60000000; ++i) {};
		yield();
	}
}
