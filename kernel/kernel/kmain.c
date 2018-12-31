#include <stdio.h>
#include <limits.h>
#include <stdint.h>
#include <ctype.h>
#include <string.h>
#include <kernel/tty.h>
#include <kernel/gdt.h>
#include <kernel/idt.h>
#include <kernel/irq.h>
#include <kernel/kprint.h>
#include <kernel/kpanic.h>
#include <mm/cache.h>
#include <mm/mmu.h>
#include <fs/initrd.h>
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

static void *init_task_func(void *arg)
{
    (void)arg;

    kdebug("starting init task...");

    for (;;) {
        kdebug("in init task!");

        for (volatile int i = 0; i < 50000000; ++i)
            ;
    }

    kdebug("ending init task...");

    return NULL;
}

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
    vfs_init(mbinfo);
    sched_init();

#if 0
    fs_t *fs = vfs_register_fs("initrd", "/mnt", mbinfo);

    if (vfs_lookup("/mnt/sbin/init") == NULL)
        kdebug("no such file or directory");
    else
        kdebug("/sbin/init exists!");
#endif

    task_t   *init_task   = sched_task_create("init_task");
    thread_t *init_thread = sched_thread_create(init_task_func, NULL);

    sched_task_add_thread(init_task, init_thread);
    sched_task_schedule(init_task);

    sched_start();

	for (;;);
}
