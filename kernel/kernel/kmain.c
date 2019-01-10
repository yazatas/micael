#include <stdint.h>
#include <string.h>
#include <kernel/tty.h>
#include <kernel/gdt.h>
#include <kernel/idt.h>
#include <kernel/irq.h>
#include <kernel/kprint.h>
#include <kernel/kpanic.h>
#include <mm/cache.h>
#include <mm/heap.h>
#include <mm/mmu.h>
#include <fs/binfmt.h>
#include <fs/fs.h>
#include <sched/task.h>
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

    /* building memory map requires multiboot info */
	vmm_init(mbinfo);

    /* initrd initialization requires multiboot info */
    vfs_init(mbinfo);
    sched_init();

    /* initialize initrd and init task */
    fs_t *fs = vfs_register_fs("initrd", "/mnt", mbinfo);

    sched_start();

	for (;;);

#if 0
    file_t *file   = NULL;
    dentry_t *dntr = NULL;

    if ((dntr = vfs_lookup("/mnt/bin/echo")) == NULL)
        kpanic("failed to find init script from file system");

    if ((file = vfs_open_file(dntr)) == NULL)
        kpanic("failed to open file /sbin/init");
#endif
}
