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
#include <mm/vmm.h>
#include <mm/kheap.h>
#include <fs/multiboot.h>
#include <fs/initrd.h>
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

extern void enter_usermode();

void **memories = NULL;
size_t file_sizes[2] = { 0 };

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

    disk_header_t *dh = initrd_init(mbinfo);
    file_header_t *fh = NULL;

    kdebug("file count: %u | disk size: %u | magic: 0x%x", 
            dh->file_count, dh->disk_size, dh->magic);

    fh = (file_header_t *)((uint8_t*)dh + sizeof(disk_header_t));

    uint8_t *mem_ptr   = NULL;
    uint8_t *ptr       = NULL;
    uint8_t *f_start_p = NULL;
    uint8_t *f_start_v = NULL;
    
    memories = kmalloc(sizeof(void *) * dh->file_count);

    for (size_t i = 0; i < dh->file_count; ++i) {

        kdebug("file start virt 0x%x", f_start_v);
        kdebug("file start phys 0x%x\n", f_start_p);
        kdebug("name '%s' | size %u | magic 0x%x", fh->file_name, fh->file_size, fh->magic);

        ptr = memories[i] = kmalloc(fh->file_size);
        file_sizes[i] = fh->file_size;

        memcpy(ptr, (uint8_t *)&fh->magic + 4, fh->file_size);

        fh = (file_header_t *)((uint8_t *)fh + sizeof(file_header_t) + fh->file_size);
    }

    kdebug("DONED FINALLY");
    kdebug("tring to jump to user mode");

    pcb_t *pcbs[2];
    pcb_t *new;

    pcbs[0] = process_create(memories[1], file_sizes[1]);
    pcbs[1] = process_create(memories[1], file_sizes[1]);
    /* pcbs[2] = process_create(memories[0], file_sizes[0]); */

    /* new = pcbs[0]; */
    /* vmm_change_pdir(new->page_dir); */
    /* enter_usermode(); */

    /* process_create(memories[0], file_sizes[0]); */

    schedule();

	for (;;);
}
