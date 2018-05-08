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

#include <kernel/task.h>
#include <kernel/mutex.h>

#include <mm/vmm.h>
#include <mm/kheap.h>

#include <sched/proc.h>

#include <drivers/timer.h>
#include <drivers/keyboard.h>

extern uint32_t __kernel_virtual_start,  __kernel_virtual_end;
extern uint32_t __kernel_physical_start, __kernel_physical_end;
extern uint32_t __code_segment_start, __code_segment_end;
extern uint32_t __data_segment_start, __data_segment_end;
extern uint32_t boot_page_dir; 

void kmain(void)
{
    tty_init_default();

	kdebug("kvirtual kphysical start: 0x%08x 0x%08x\n"
		   "[kmain] kvirtual kphysical end:   0x%08x 0x%08x",
		   &__kernel_virtual_start, &__kernel_physical_start,
		   &__kernel_virtual_end,   &__kernel_physical_end);
	kdebug("code segment start - end: 0x%08x - 0x%08x",
			&__code_segment_start, &__code_segment_end);
	kdebug("data segment start - end: 0x%08x - 0x%08x",
			&__data_segment_start, &__data_segment_end);
	kdebug("kpage dir start addr:     0x%08x", &boot_page_dir);

	gdt_init(); 
	idt_init(); 
	irq_init(); 
	timer_install(); kb_install();
	asm ("sti"); /* enable interrupts */
	vmm_init();

	vmm_list_pde();

	pcb_t *p;
	if ((p = process_create("haloust")) != NULL) {
		kdebug("process created successfully");
		kdebug("pid %u", p->pid);
	} else {
		kdebug("failed to create process");
	}

	for (;;);
}
