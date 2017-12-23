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

extern uint32_t __kernel_virtual_start,  __kernel_virtual_end;
extern uint32_t __kernel_physical_start, __kernel_physical_end;
extern uint32_t boot_page_dir; 

void kmain(void)
{
    tty_init_default();

	kprint("kvirtual kphysical start: 0x%08x 0x%08x\n"
		   "kvirtual kphysical end:   0x%08x 0x%08x\n",
		   &__kernel_virtual_start, &__kernel_physical_start,
		   &__kernel_virtual_end,   &__kernel_physical_end);
	kprint("kpage dir start addr:     0x%08x\n", &boot_page_dir);

	gdt_init(); idt_init();
	irq_init(); timer_install();
	kb_install();
	asm ("sti"); /* enable interrupts */
	mmu_init();

	kprint("\n\n");

	uint32_t test;

	test = kalloc_frame();
	kprint("alloc 1 address 0x%0x\n\n", test);

	test = kalloc_frame();
	kprint("alloc 2 address 0x%0x\n\n", test);

	kfree_frame(test);

	test = kalloc_frame();
	kprint("alloc 3 address 0x%0x\n\n", test);

	test = kalloc_frame();
	kprint("alloc 4 address 0x%0x\n\n", test);

	test = kalloc_frame();
	kprint("alloc 5 address 0x%0x\n\n", test);


	for (;;);
}
