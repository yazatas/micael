#include <stdio.h>
#include <limits.h>
#include <stdint.h>
#include <string.h>

#include <kernel/tty.h>
#include <kernel/gdt.h>
#include <kernel/idt.h>
#include <kernel/irq.h>
#include <kernel/kprint.h>
#include <kernel/kpanic.h>
#include <kernel/mmu.h>
#include <kernel/kheap.h>

#include <kernel/drivers/timer.h>
#include <kernel/drivers/keyboard.h>

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
	traverse();

	void *tmp = kmalloc(100);
	void *ptr = kmalloc(10);

	char *str = "test123";
	memcpy(ptr, str, strlen(str));;

	kprint("'%s'\n", ptr);
	traverse();

	kfree(tmp);
	traverse();
	ptr = krealloc(ptr, 8000);

	kprint("'%s'\n", ptr);

	traverse();
	for (;;);
}
