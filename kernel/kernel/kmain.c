#include <stdint.h>
#include <string.h>
#include <drivers/vbe.h>
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

void kmain(multiboot_info_t *mbinfo)
{
    gdt_init(); idt_init(); irq_init();
    kb_install();

    /* building memory map requires multiboot info */
    mmu_init(mbinfo);

    /* VBE must be initialized after MMU, because it uses
     * mmu functions to get the from from VGA memory */
    vbe_init();

    /* initrd initialization requires multiboot info */
    vfs_init(mbinfo);

    /* create init and idle tasks */
    sched_init();

    fs_t *fs = vfs_register_fs("initrd", "/mnt", mbinfo);

    file_t *file    = NULL;
    dentry_t *dntr  = NULL;
    uint8_t arr[100] = { 0 };

    if ((dntr = vfs_lookup("/mnt/bin/echo")) == NULL)
        kpanic("failed to find init script from file system");

    if ((file = vfs_open_file(dntr)) == NULL)
        kpanic("failed to open file /sbin/init");

    if (vfs_read(file, 0, 100, arr) < 0) {
        kdebug("failed to read from memroy");
    }

    hex_dump(arr, 100);

    sched_start();

    for (;;);
}
