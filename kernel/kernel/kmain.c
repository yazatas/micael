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

	kthread_create(func_3, 0x500, "func_3");
	kthread_create(func_2, 0x500, "func_2");
	kthread_create(func_1, 0x500, "func_1");
	kthread_start();

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
		kthread_yield();
	}
}

void func_2(void)
{
	kdebug("started func_2");

	for (int i = 0; i < 3; ++i) {
		kprint("in function 2\n");
		for (volatile int i = 0; i < 60000000; ++i) {};
		kthread_yield();
	}

	kthread_delete();
}

void func_3(void)
{
	kdebug("started func_3");

	while (1) {
		kprint("in function 3\n");
		for (volatile int i = 0; i < 60000000; ++i) {};
		kthread_yield();
	}
}
