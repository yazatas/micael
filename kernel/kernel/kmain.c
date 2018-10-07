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
#include <sync/mutex.h>
#include <mm/cache.h>
#include <mm/kheap.h>
#include <mm/vmm.h>
#include <fs/multiboot.h>
#include <fs/initrd.h>
#include <fs/vfs.h>
#include <sched/kthread.h>
#include <sched/proc.h>
#include <sched/sched.h>
#include <drivers/timer.h>
#include <drivers/keyboard.h>

extern uint32_t __kernel_virtual_start,  __kernel_virtual_end;
extern uint32_t __kernel_physical_start, __kernel_physical_end;
extern uint32_t __code_segment_start, __code_segment_end;
extern uint32_t __data_segment_start, __data_segment_end;
extern uint32_t boot_page_dir; 

void kmain(multiboot_info_t *mbinfo)
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

	gdt_init(); idt_init(); irq_init(); 
	timer_install(); kb_install();
    enable_irq();

	vmm_init(mbinfo);
    (void)cache_init();
    vmm_print_memory_map();

    void *ptr;

    for (int i = 0; i < 5; ++i) {
        if ((ptr = cache_alloc(C_NOFLAGS)) == NULL)
            kdebug("dick space error");

        /* cache_print_list(0); */
        /* cache_print_list(1); */

        /* cache_dealloc(ptr, C_NOFLAGS); */

        /* kprint("\n"); */

        /* cache_print_list(0); */
        /* cache_print_list(1); */
    }

    cache_print_list(0);
    cache_print_list(1);

    cache_dealloc(ptr, C_NOFLAGS);

    cache_print_list(0);
    cache_print_list(1);

	for (;;);
}
